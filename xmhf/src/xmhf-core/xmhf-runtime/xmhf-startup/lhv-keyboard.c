#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define KEYBOARD_PORT 0x60

void handle_keyboard_interrupt(VCPU *vcpu, int vector)
{
	uint8_t scancode = inb(KEYBOARD_PORT);
	(void) vector;
	printf("\nCPU(0x%02x): key press: %d", vcpu->id, (int)scancode);
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}
