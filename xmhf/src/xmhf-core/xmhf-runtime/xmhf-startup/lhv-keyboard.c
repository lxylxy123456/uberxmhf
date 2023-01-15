#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define KEYBOARD_PORT 0x60

volatile u32 drop_keyboard_interrupts = 0;

void handle_keyboard_interrupt(VCPU *vcpu, int vector, int guest)
{
	HALT_ON_ERRORCOND(vcpu->isbsp);
	HALT_ON_ERRORCOND(vector == 0x21);
	if (drop_keyboard_interrupts) {
		drop_keyboard_interrupts--;
		printf("Dropped a keyborad interrupt\n");
	} else {
		uint8_t scancode = inb(KEYBOARD_PORT);
		printf("CPU(0x%02x): key press: 0x%hh02x, guest=%d\n", vcpu->id,
			   scancode, guest);
	}
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

