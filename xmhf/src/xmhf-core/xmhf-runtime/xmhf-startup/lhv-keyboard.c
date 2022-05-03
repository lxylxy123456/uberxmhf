#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define KEYBOARD_PORT 0x60

void handle_keyboard_interrupt(void)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	uint8_t scancode = inb(KEYBOARD_PORT);
	printf("\nCPU(0x%02x): key press: %d", vcpu->id, (int)scancode);
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}
