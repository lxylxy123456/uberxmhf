#include <xmhf.h>
#include <lhv.h>
#include "trustvisor.h"

/* Stack for user program */
static u8 user_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));

/* Stack for interrupt and system call during user mode */
static u8 interrupt_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));

/* When running user program, ESP of kernel code (used when user exits) */
static uintptr_t esp0[MAX_VCPU_ENTRIES];

#ifdef __AMD64__
static u64 user_pml4t[P4L_NPLM4T * 512] __attribute__((aligned(PAGE_SIZE_4K)));
static u64 user_pdpt[P4L_NPDPT * 512] __attribute__((aligned(PAGE_SIZE_4K)));
static u64 user_pdt[P4L_NPDT * 512] __attribute__((aligned(PAGE_SIZE_4K)));
static u64 user_pt[P4L_NPT * 512] __attribute__((aligned(PAGE_SIZE_4K)));
#elif defined(__I386__)
static u64 user_pdpt[4] __attribute__ ((aligned (32)));
static u64 user_pd[4][512] __attribute__((aligned(PAGE_SIZE_4K)));
static u64 user_pt[4][512][512] __attribute__((aligned(PAGE_SIZE_4K)));
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

static void set_user_mode_page_table(void)
{
	static uintptr_t initialized = 0;
	static u32 lock = 1;
	spin_lock(&lock);
	if (!initialized) {
#ifdef __AMD64__
		u64 i = 0;
		u64 paddr = (uintptr_t) user_pdpt;
		for (i = 0; i < P4L_NPDPT; i++) {
			user_pml4t[i] = 7 | paddr;
			paddr += PAGE_SIZE_4K;
		}
		paddr = (uintptr_t) user_pdt;
		for (i = 0; i < P4L_NPDT; i++) {
			user_pdpt[i] = 7 | paddr;
			paddr += PAGE_SIZE_4K;
		}
		paddr = (uintptr_t) user_pt;
		for (i = 0; i < P4L_NPT; i++) {
			user_pdt[i] = 7 | paddr;
			paddr += PAGE_SIZE_4K;
		}
		paddr = 0;
		for (i = 0;
			 i < PA_PAGE_ALIGN_UP_NOCHK_4K(MAX_PHYS_ADDR) >> PAGE_SHIFT_4K;
			 i++) {
			user_pt[i] = 7 | paddr;
			paddr += PAGE_SIZE_4K;
		}
		initialized = (uintptr_t) user_pml4t;
#elif defined(__I386__)
		u32 i, j, k;
		u64 paddr = 0;
		for (i = 0; i < 4; i++) {
			user_pdpt[i] = 1 | (uintptr_t) user_pd[i];
			for (j = 0; j < 512; j++) {
				user_pd[i][j] = 7 | (uintptr_t) user_pt[i][j];
				for (k = 0; k < 512; k++) {
					user_pt[i][j][k] = 7 | paddr;
					paddr += PA_PAGE_SIZE_4K;
				}
			}
		}
		initialized = (uintptr_t) user_pdpt;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	}
	write_cr3(initialized);
	spin_unlock(&lock);
}

/* arg indicates VMCALL EAX offset. Used by pal_demo code during nested virt */
void enter_user_mode(VCPU *vcpu, ulong_t arg)
{
	uintptr_t stack_top = ((uintptr_t) user_stack[vcpu->idx]) + PAGE_SIZE_4K;
	uintptr_t *stack = (uintptr_t *)stack_top;
	uintptr_t *pesp0 = &esp0[vcpu->idx];
	ureg_t *ureg = (ureg_t *)((uintptr_t)(&stack[-3]) - sizeof(ureg_t));
	memset(&ureg->r, 0, sizeof(ureg->r));
	ureg->eip = (uintptr_t) user_main;
	ureg->cs = __CS_R3;
	ureg->eflags = 2 | (3 << 12);
	ureg->esp = (uintptr_t) (&stack[-3]);
	ureg->ss = __DS_R3;
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		ureg->eflags |= EFLAGS_IF;
	}
	{
		uintptr_t top = ((uintptr_t) interrupt_stack[vcpu->idx]) + PAGE_SIZE_4K;
#ifdef __AMD64__
		ureg->r.rsi = arg;
		ureg->r.rdi = (uintptr_t) vcpu;
		stack[-3] = 0xdeadbeef;
		/* Setup TSS */
		*(u64 *)(g_runtime_TSS[vcpu->idx] + 4) = top;
#elif defined(__I386__)
		stack[-1] = arg;
		stack[-2] = (uintptr_t) vcpu;
		stack[-3] = 0xdeadbeef;
		/* Setup TSS */
		*(u32 *)(g_runtime_TSS[vcpu->idx] + 4) = top;
		*(u16 *)(g_runtime_TSS[vcpu->idx] + 8) = __DS;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	}
	/* Setup page table */
	set_user_mode_page_table();
	/* Iret to user mode */
	enter_user_mode_asm(ureg, pesp0);
}

void handle_lhv_syscall(VCPU *vcpu, int vector, struct regs *r)
{
	/* Currently the only syscall is to exit guest mode */
	HALT_ON_ERRORCOND(vector == 0x23);
	HALT_ON_ERRORCOND(r->eax == 0xdeaddead);
	asm volatile ("push $0; push $0; lidt (%rsp); ud2;");
	exit_user_mode_asm(esp0[vcpu->idx]);
}

/* Below are for pal_demo */

static u8 pal_demo_code[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));
static u8 pal_demo_data[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));
static u8 pal_demo_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));
static u8 pal_demo_param[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__((aligned(PAGE_SIZE_4K)));

void begin_pal_c(void) {}

uintptr_t my_pal(uintptr_t arg1, uintptr_t arg2) {
#if 0
	u32 eax=0x7a567254U, ebx, ecx=arg2, edx;
	cpuid_raw(&eax, &ebx, &ecx, &edx);
	if (eax != 0x7a767274U) {
		return 0xdeadbee1U;
	}
	if (ebx == 0xffffffffU) {
		return 0xdeadbee2U;
	}
#endif
	(void)arg1;
	return *(uintptr_t *)arg2;
}

void end_pal_c(void) {}

static inline uintptr_t vmcall(uintptr_t eax, uintptr_t ecx, uintptr_t edx,
								uintptr_t esi, uintptr_t edi) {
	asm volatile ("vmcall\n\t" : "=a"(eax) : "a"(eax), "c"(ecx), "d"(edx),
					"S"(esi), "D"(edi));
	return eax;
}

/* Above are for pal_demo */

__attribute__((__noreturn__)) void leave_user_mode(void) {
	asm volatile ("movl $0xdeaddead, %eax; int $0x23;");
	HALT_ON_ERRORCOND(0 && "system call returned");
}

static u32 pal_lock = 1;
static volatile u32 pal_available = 3;

static void user_main_pal_demo(VCPU *vcpu, ulong_t arg)
{
	struct tv_pal_sections sections = {
		num_sections: 4,
		sections: {
			{ TV_PAL_SECTION_CODE, 1, (uintptr_t) pal_demo_code[vcpu->idx] },
			{ TV_PAL_SECTION_DATA, 1, (uintptr_t) pal_demo_data[vcpu->idx] },
			{ TV_PAL_SECTION_STACK, 1, (uintptr_t) pal_demo_stack[vcpu->idx] },
			{ TV_PAL_SECTION_PARAM, 1, (uintptr_t) pal_demo_param[vcpu->idx] }
		}
	};
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 }, { TV_PAL_PM_INTEGER, 4 }
		}
	};
	/* Copy code */
	uintptr_t src_begin = (uintptr_t) begin_pal_c;
	uintptr_t dst_begin = (uintptr_t) pal_demo_code[vcpu->idx];
	uintptr_t code_size = (uintptr_t) end_pal_c - src_begin;
	uintptr_t pal_entry = dst_begin - src_begin + (uintptr_t) my_pal;
	typeof(&my_pal) pal_func = (typeof(&my_pal)) pal_entry;
	memcpy((void *)dst_begin, (void *)src_begin, code_size);

	/*
	 * Touch all pages used by PAL. If in nested virtualization (L0 = XMHF,
	 * L1 = red hypervisor, L2 = LHV), hopefully the pages become cached in
	 * L1's EPT.
	 */
	for (u32 i = 0; i < sections.num_sections; i++) {
		for (u32 j = 0; j < sections.sections[i].page_num; j++) {
			u64 addr = sections.sections[i].start_addr + PAGE_SIZE_4K * j;
			volatile u8 *ptr = (volatile u8 *)(uintptr_t)addr;
			*ptr;
		}
	}

	/* Register PAL */
	printf("CPU(0x%02x): starting PAL 0x%x\n", vcpu->id, arg);
	HALT_ON_ERRORCOND(!vmcall(TV_HC_REG + arg, (uintptr_t)&sections, 0,
							  (uintptr_t)&params, pal_entry));

	/* Call PAL function */
	{
		uintptr_t ans = pal_func(0x11111111, arg);
		if (ans != 0x2345bcde) {
			printf("CPU(0x%02x): PAL returns incorrect result\n", vcpu->id);
			printf("CPU(0x%02x): Expected: 0x%08lx\n", vcpu->id, 0x2345bcde);
			printf("CPU(0x%02x): Actual:   0x%08lx\n", vcpu->id, ans);
			spin_lock(&pal_lock);
			printf("CPU(0x%02x): Locked pal_lock\n", vcpu->id);
			while (1) {
				xmhf_cpu_relax();
			}
		}
	}

	if (0 && "invalid access") {
		printf("", *(u32*)pal_entry);
	}

	/* Unregister PAL */
	HALT_ON_ERRORCOND(!vmcall(TV_HC_UNREG + arg, pal_entry, 0, 0, 0));

	if ("valid access") {
		printf("", *(u32*)pal_entry);
	}

	printf("CPU(0x%02x): completed PAL 0x%x\n", vcpu->id, arg);
}

#define XMHF_ADDR 0x10003000

static void xxd(uintptr_t start, uintptr_t end) {
	if ((start & 0xf) != 0 || (end & 0xf) != 0) {
		HALT_ON_ERRORCOND(0);
		//printf("xxd assertion failed");
		//while (1) {
		//	asm volatile ("hlt");
		//}
	}
	for (uintptr_t i = start; i < end; i += 0x10) {
		printf("XXD: %08lx: ", i);
		for (uintptr_t j = 0; j < 0x10; j++) {
			if (j & 1) {
				printf("%02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			} else {
				printf(" %02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			}
		}
		printf("\n");
	}
}

static void user_main_pal_demo_2(VCPU *vcpu, ulong_t arg)
{
	struct tv_pal_sections sections = {
		num_sections: 4,
		sections: {
			{ TV_PAL_SECTION_CODE, 1, (uintptr_t) pal_demo_code[arg] },
			{ TV_PAL_SECTION_DATA, 1, (uintptr_t) XMHF_ADDR },
			{ TV_PAL_SECTION_STACK, 1, (uintptr_t) pal_demo_stack[arg] },
			{ TV_PAL_SECTION_PARAM, 1, (uintptr_t) pal_demo_param[arg] }
		}
	};
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 }, { TV_PAL_PM_INTEGER, 4 }
		}
	};
	/* Copy code */
	uintptr_t src_begin = (uintptr_t) begin_pal_c;
	uintptr_t dst_begin = (uintptr_t) pal_demo_code[arg];
	uintptr_t code_size = (uintptr_t) end_pal_c - src_begin;
	uintptr_t pal_entry = dst_begin - src_begin + (uintptr_t) my_pal;
	typeof(&my_pal) pal_func = (typeof(&my_pal)) pal_entry;
	memcpy((void *)dst_begin, (void *)src_begin, code_size);

	/*
	 * Touch all pages used by PAL. If in nested virtualization (L0 = XMHF,
	 * L1 = red hypervisor, L2 = LHV), hopefully the pages become cached in
	 * L1's EPT.
	 */
//	for (u32 i = 0; i < sections.num_sections; i++) {
//		for (u32 j = 0; j < sections.sections[i].page_num; j++) {
//			u64 addr = sections.sections[i].start_addr + PAGE_SIZE_4K * j;
//			volatile u8 *ptr = (volatile u8 *)(uintptr_t)addr;
//			*ptr;
//		}
//	}

	/* Register PAL */
	printf("CPU(0x%02x): starting PAL 0x%x\n", vcpu->id, arg);
	HALT_ON_ERRORCOND(!vmcall(TV_HC_REG, (uintptr_t)&sections, 0,
							  (uintptr_t)&params, pal_entry));

	/* Call PAL function */
	for (int i = 0; i < 4096; i += sizeof(uintptr_t)) {
		uintptr_t val = pal_func(0, XMHF_ADDR + i);
		//printf("Read %d, get 0x%08lx\n", i, val);
		*(uintptr_t *)(((uintptr_t)(pal_demo_data[0])) + i) = val;
	}

	xxd((uintptr_t) pal_demo_data[0], ((uintptr_t) pal_demo_data[0]) + 4096);
	for (int i = 0; i < 4096; i += 16) {
		printf("0x%08lx:\t0x%016llx\t0x%016llx\n", XMHF_ADDR + i,
			   *(uintptr_t *)(((uintptr_t) pal_demo_data[0]) + i),
			   *(uintptr_t *)(((uintptr_t) pal_demo_data[0]) + i + 8));
	}
	printf("XMHF_ADDR = 0x%08lx\n", XMHF_ADDR);
	printf("x/512gx 0x%08lx\n", XMHF_ADDR);

	if (0 && "invalid access") {
		printf("", *(u32*)pal_entry);
	}

//	/* Unregister PAL */
//	HALT_ON_ERRORCOND(!vmcall(TV_HC_UNREG, pal_entry, 0, 0, 0));

//	if ("valid access") {
//		printf("", *(u32*)pal_entry);
//	}

	printf("CPU(0x%02x): completed PAL 0x%x\n", vcpu->id, arg);
}

void user_main(VCPU *vcpu, ulong_t arg)
{
	/* Acquire semaphore */
	{
		while (1) {
			spin_lock(&pal_lock);
			if (pal_available) {
				pal_available--;
				spin_unlock(&pal_lock);
				break;
			} else {
				u32 i;
				spin_unlock(&pal_lock);
				for (i = 0; i < 4096 && !pal_available; i++) {
					asm volatile ("pause");		/* Save energy when waiting */
				}
			}
		}
	}
	/* Run pal_demo if in XMHF */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(0x46484d58U, &eax, &ebx, &ecx, &edx);
		//if (eax == 0x46484d58U) {
		if (0) {
			/* Test TrustVisor presence using CPUID */
			u32 eax=0x7a567254U, ebx, ecx=arg, edx;
			cpuid_raw(&eax, &ebx, &ecx, &edx);
			//HALT_ON_ERRORCOND(eax == 0x7a767274U && "TrustVisor not present 1");
			//HALT_ON_ERRORCOND(ebx == 0xffffffffU && "TrustVisor not present 2");
			user_main_pal_demo(vcpu, arg);
		} else {
			printf("CPU(0x%02x): can enter user mode 0x%x\n", vcpu->id, arg);
			user_main_pal_demo_2(vcpu, 0);
		}
	}
	/* Release semaphore */
	spin_lock(&pal_lock);
	pal_available++;
	spin_unlock(&pal_lock);
	leave_user_mode();
}

