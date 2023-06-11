/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

