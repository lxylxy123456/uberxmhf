#include <xmhf.h>
#include <lhv.h>
#include "trustvisor.h"

u8 user_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

u64 user_pdpt[MAX_VCPU_ENTRIES][4]
__attribute__ ((aligned (32)));

u64 user_pd[MAX_VCPU_ENTRIES][4][512]
__attribute__(( section(".bss.palign_data") ));

u64 user_pt[MAX_VCPU_ENTRIES][4][512][512]
__attribute__(( section(".bss.palign_data") ));

void enter_user_mode(VCPU *vcpu, ulong_t arg)
{
#ifdef __AMD64__
	(void) vcpu;
	(void) arg;
	HALT_ON_ERRORCOND(0 && "64-bit not supported (yet?)");
#else
	uintptr_t stack_top = ((uintptr_t) user_stack[vcpu->idx]) + PAGE_SIZE_4K;
	u32 *stack = (u32 *)stack_top;
	u32 i, j, k;
	u64 paddr = 0;
	ureg_t ureg = {
		.edi=0,
		.esi=0,
		.ebp=0,
		.zero=0,
		.ebx=0,
		.edx=0,
		.ecx=0,
		.eax=arg,
		.eip=(uintptr_t) user_main,
		.cs=0x2b,
		.eflags=2 | (3 << 12),
		.esp=(uintptr_t) (&stack[-3]),
		.ss=0x33,
	};
	stack[-1] = arg;
	stack[-2] = (uintptr_t) vcpu;
	stack[-3] = 0xdeadbeef;
	/* Setup page table */
	for (i = 0; i < 4; i++) {
		user_pdpt[vcpu->idx][i] = 1 | (uintptr_t) user_pd[vcpu->idx][i];
		for (j = 0; j < 512; j++) {
			user_pd[vcpu->idx][i][j] = 7 | (uintptr_t) user_pt[vcpu->idx][i][j];
			for (k = 0; k < 512; k++) {
				user_pt[vcpu->idx][i][j][k] = 7 | paddr;
				paddr += PA_PAGE_SIZE_4K;
			}
		}
	}
	write_cr3((uintptr_t) user_pdpt[vcpu->idx]);
	enter_user_mode_asm(&ureg);
#endif
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

static u32 state_lock = 1;
static u32 state0;
static u32 state1;
uintptr_t pal_addr;

void user_main_cpu0(VCPU *vcpu, ulong_t arg)
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

	// pal_addr = pal_entry;
	pal_addr = (uintptr_t) pal_demo_stack[vcpu->idx];

	if (0 && "invalid access") {
		printf("", *(u32*)pal_entry);
	}

	HALT_ON_ERRORCOND(!vmcall(TV_HC_UNREG, pal_entry, 0, 0, 0));

	if ("valid access") {
		printf("", *(u32*)pal_entry);
	}

	printf("LXY 0 completed first PAL\n");

	spin_lock(&state_lock);
	state0 = 1;
	spin_unlock(&state_lock);

	while (1) {
		u32 s;
		spin_lock(&state_lock);
		s = state1;
		spin_unlock(&state_lock);
		if (s >= 1) {
			break;
		}
	}

	printf("LXY 0 starting second PAL\n");

	HALT_ON_ERRORCOND(!vmcall(
		TV_HC_REG, (uintptr_t)&sections, 0, (uintptr_t)&params, pal_entry));

	printf("LXY 0 registered second PAL");

	spin_lock(&state_lock);
	state0 = 2;
	spin_unlock(&state_lock);

	for (u32 i = 0; ; i += 10) {
		if (i == 0) {
			printf("0 ended\n");
		}
	}
	(void)arg;
}

void user_main_cpu1(VCPU *vcpu, ulong_t arg)
{
	while (1) {
		u32 s;
		spin_lock(&state_lock);
		s = state0;
		spin_unlock(&state_lock);
		if (s >= 1) {
			break;
		}
	}

	printf("LXY 1 starting heating TLB\n");

	HALT_ON_ERRORCOND(pal_addr);
	for (u32 i = 0; i < 10000; i++) {
		if (*(volatile u32 *)pal_addr == 0) {
			continue;
		}
	}

	printf("LXY 1 heated TLB\n");

	spin_lock(&state_lock);
	state1 = 1;
	spin_unlock(&state_lock);

	while (1) {
		u32 s;
		spin_lock(&state_lock);
		s = state0;
		spin_unlock(&state_lock);
		if (s >= 2) {
			break;
		}
	}

	printf("LXY 1 accessing second PAL\n");

	for (u32 i = 0; i < 10; i++) {
		if (*(volatile u32 *)pal_addr == 0) {
			continue;
		}
	}



	for (u32 i = 0; ; i += 10) {
		if (i == 0) {
			printf("1 ended\n");
		}
	}
	(void)vcpu;
	(void)arg;
}

void user_main(VCPU *vcpu, ulong_t arg)
{
	switch (vcpu->idx) {
	case 0:
		user_main_cpu0(vcpu, arg);
		break;
	case 1:
		user_main_cpu1(vcpu, arg);
		break;
	default:
		break;
	}
	HALT();
}

