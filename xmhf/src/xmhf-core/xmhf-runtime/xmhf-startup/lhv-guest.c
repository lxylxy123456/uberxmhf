#include <xmhf.h>
#include <lhv.h>

extern u8 lxy[PAGE_SIZE_4K];

void lhv_guest_main(ulong_t cpu_id)
{
	if (cpu_id == 0) {
		while (1) {
			asm volatile ("vmcall");
			lxy[0]++;
		}
	} else {
		while (1) {
			printf("%ld access lxy\n", cpu_id);
			lxy[0]++;
		}
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
