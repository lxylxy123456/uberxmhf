#include <xmhf.h>
#include <lhv.h>
#include "trustvisor.h"

u8 user_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

void enter_user_mode(VCPU *vcpu, ulong_t arg)
{
	uintptr_t stack_top = ((uintptr_t) user_stack[vcpu->idx]) + PAGE_SIZE_4K;
	uintptr_t *stack = (uintptr_t *)stack_top;
	ureg_t ureg = {
		.r={},
		.eip=(uintptr_t) user_main,
		.cs=__CS_R3,
		.eflags=2 | 0x200 | (3 << 12),
		.esp=(uintptr_t) (&stack[-3]),
		.ss=__DS_R3,
	};
	memset(&ureg.r, 0, sizeof(ureg.r));
#ifdef __AMD64__
	ureg.r.rsi = arg;
	ureg.r.rdi = (uintptr_t) vcpu;
	stack[-3] = 0xdeadbeef;
	/* Setup TSS */
	*(u64 *)(g_runtime_TSS[vcpu->idx] + 4) = vcpu->rsp;
#elif defined(__I386__)
	stack[-1] = arg;
	stack[-2] = (uintptr_t) vcpu;
	stack[-3] = 0xdeadbeef;
	/* Setup TSS */
	*(u32 *)(g_runtime_TSS[vcpu->idx] + 4) = vcpu->esp;
	*(u16 *)(g_runtime_TSS[vcpu->idx] + 8) = __DS;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	/* Iret to user mode */
	enter_user_mode_asm(&ureg);
}

/* Below are for pal_demo */

void begin_pal_c(void) {}

uintptr_t my_pal(uintptr_t arg1) {
	return arg1 + 0x1234abcd;
}

void end_pal_c(void) {}

u8 pal_demo_code[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));
u8 pal_demo_data[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));
u8 pal_demo_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));
u8 pal_demo_param[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

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

void user_main_pal_demo(VCPU *vcpu, ulong_t arg)
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
		num_params: 1,
		params: {
			{ TV_PAL_PM_INTEGER, 4 }
		}
	};
	/* Copy code */
	uintptr_t src_begin = (uintptr_t) begin_pal_c;
	uintptr_t dst_begin = (uintptr_t) pal_demo_code[vcpu->idx];
	uintptr_t code_size = (uintptr_t) end_pal_c - src_begin;
	uintptr_t pal_entry = dst_begin - src_begin + (uintptr_t) my_pal;
	typeof(&my_pal) pal_func = (typeof(&my_pal)) pal_entry;
	memcpy((void *)dst_begin, (void *)src_begin, code_size);
	HALT_ON_ERRORCOND(!vmcall(
		TV_HC_REG, (uintptr_t)&sections, 0, (uintptr_t)&params, pal_entry));
	HALT_ON_ERRORCOND(pal_func(0x11111111) == 0x2345bcde);

	if (0 && "invalid access") {
		printf("", *(u32*)pal_entry);
	}

	HALT_ON_ERRORCOND(!vmcall(TV_HC_UNREG, pal_entry, 0, 0, 0));

	if ("valid access") {
		printf("", *(u32*)pal_entry);
	}

	printf("CPU(0x%02x): completed PAL\n", vcpu->id);
	(void)arg;
}

void user_main(VCPU *vcpu, ulong_t arg)
{
	leave_user_mode();
	user_main_pal_demo(vcpu, arg);
}

