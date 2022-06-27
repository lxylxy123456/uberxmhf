#include <xmhf.h>
#include <lhv.h>

u8 user_stack[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

u64 user_pdpt[MAX_VCPU_ENTRIES][4]
__attribute__ ((aligned (32)));

u64 user_pd[MAX_VCPU_ENTRIES][4][512]
__attribute__(( section(".bss.palign_data") ));

u64 user_pt[MAX_VCPU_ENTRIES][4][512][512]
__attribute__(( section(".bss.palign_data") ));

void user_main(ulong_t arg);

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
		.eflags=2,
		.esp=(uintptr_t) (&stack[-2]),
		.ss=0x33,
	};
	stack[-1] = arg;
	stack[-2] = 0xdeadbeef;
	enter_user_mode_asm(&ureg);
#endif
}

void user_main(ulong_t arg)
{
	while (1);
	(void)arg;
}
