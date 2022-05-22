#include <xmhf.h>
#include <lhv.h>

void lhv_guest_main(ulong_t cpu_id)
{
	asm volatile ("sti");
	while (1) {
		asm volatile ("cli");
		(void) cpu_id;
//		printf("%d", cpu_id);
		asm volatile ("sti");
	}
}

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	(void) r;
	switch (vector) {
	case 0x20:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector);
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector);
		break;
	case 0x22:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector);
		break;
	default:
		while (1) {
			printf("\nGuest: unknown interrupt / exception!");
		}
		break;
	}
}
