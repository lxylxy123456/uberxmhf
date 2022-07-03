#include <xmhf.h>
#include <lhv.h>

void lhv_guest_main(ulong_t cpu_id)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	console_vc_t vc;
	HALT_ON_ERRORCOND(cpu_id == vcpu->idx);
	console_get_vc(&vc, vcpu->idx, 1);
	console_clear(&vc);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	while (1) {
		// asm volatile ("hlt");
		// asm volatile ("vmcall");
#if 0
		if (__LHV_OPT__ & LHV_USE_EPT) {
			u32 a = 0xdeadbeef;
			u32 *p = (u32 *)0x12340000;
			printf("!ACCESS\n");
			// asm volatile("movl (%1), %%eax" :
			asm volatile("vmcall" :
						 "+a" (a) :
						 "b" (p) :
						 "cc", "memory");
			HALT_ON_ERRORCOND(a == 0xfee1c0de);
		}
#endif
		asm volatile ("sti");
		asm volatile ("hlt");
		asm volatile ("cli");
	}
}

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	(void) r;
	switch (vector) {
	case 0x20:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x22:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	default:
//		while (1) {
//			printf("Guest: unknown interrupt / exception!\n");
//		}
		break;
	}
}
