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
	enter_user_mode_asm(&ureg);
#endif
}

/* Below are for pal_demo */

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

void user_main(VCPU *vcpu, ulong_t arg)
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
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	uintptr_t pal_entry = 0;	// TODO
	HALT_ON_ERRORCOND(!vmcall(
		TV_HC_REG, (uintptr_t)&sections, 0, (uintptr_t)&params, pal_entry));

	while (1) printf(".");
	(void)arg;
}
