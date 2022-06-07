#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define KEYBOARD_PORT 0x60

void handle_keyboard_interrupt(VCPU *vcpu, int vector, int guest)
{
	uint8_t scancode = inb(KEYBOARD_PORT);
	(void) vector;
	printf("CPU(0x%02x): key press: %d, guest=%d\n", vcpu->id, (int)scancode,
			guest);
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}
