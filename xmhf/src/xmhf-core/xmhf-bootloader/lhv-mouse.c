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

#define PS2_DATA_PORT	0x60
#define PS2_CTRL_PORT	0x64

volatile u32 drop_mouse_interrupts = 0;

/* Read from status register */
static u8 ps2_recv_stat(void)
{
	return inb(PS2_CTRL_PORT);
}

/* Write to command register */
static void ps2_send_ctrl(u8 ctrl)
{
	while (ps2_recv_stat() & 2) {
		xmhf_cpu_relax();
	}
	outb(ctrl, PS2_CTRL_PORT);
}

/* Read from data port */
static u8 ps2_recv_data(void)
{
	while (!(ps2_recv_stat() & 1)) {
		xmhf_cpu_relax();
	}
	return inb(PS2_DATA_PORT);
}

/* Write to data port */
static void ps2_send_data(u8 data)
{
	while (ps2_recv_stat() & 2) {
		xmhf_cpu_relax();
	}
	outb(data, PS2_DATA_PORT);
}

/* Read from configuration byte */
static u8 ps2_read_conf(void)
{
	ps2_send_ctrl(0x20);
	return ps2_recv_data();
}

/* Write to configuration byte */
static void ps2_write_conf(u8 data)
{
	ps2_send_ctrl(0x60);
	ps2_send_data(data);
}

void mouse_init(VCPU *vcpu)
{
	HALT_ON_ERRORCOND(vcpu->isbsp);
	printf("Initializing mouse\n");

	/*
	 * Initialize PS/2 controller.
	 * Mostly following https://wiki.osdev.org/%228042%22_PS/2_Controller
	 */

	/* Step 3: Disable Devices */
	ps2_send_ctrl(0xad);
	ps2_send_ctrl(0xa7);

	/* Step 4: Flush The Output Buffer */
	while (ps2_recv_stat() & 1) {
		ps2_recv_data();
	}

	/* Step 5: Set the Controller Configuration Byte */
	{
		u8 conf = ps2_read_conf();
		/* If fail, not dual channel PS/2 controller */
		HALT_ON_ERRORCOND((conf & 0x20));
		ps2_write_conf(conf & ~0x43);
	}

	/* Step 6: Perform Controller Self Test */
	ps2_send_ctrl(0xaa);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0x55);

	/* Step 7: Determine If There Are 2 Channels */
	ps2_send_ctrl(0xa8);
	HALT_ON_ERRORCOND(!(ps2_read_conf() & (1U << 5)));
	ps2_send_ctrl(0xa7);
	HALT_ON_ERRORCOND(ps2_read_conf() & (1U << 5));

	/* Step 8: Perform Interface Tests */
	ps2_send_ctrl(0xab);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0x00);
	ps2_send_ctrl(0xa9);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0x00);

	/* Step 9: Enable Devices */
	ps2_send_ctrl(0xae);
	ps2_send_ctrl(0xa8);
	{
		u8 conf = ps2_read_conf();
		HALT_ON_ERRORCOND(!(conf & 0x30));
		ps2_write_conf(conf | 0x03);
	}

#if 0
	/* Step 10: Reset Devices */
	drop_mouse_interrupts = ???;
	drop_keyboard_interrupts = ???;
	ps2_send_data(0xff);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0xfa);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0xaa);
	ps2_send_ctrl(0xd4);
	ps2_send_data(0xff);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0xfa);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0xaa);
	HALT_ON_ERRORCOND(drop_mouse_interrupts == 0);
	HALT_ON_ERRORCOND(drop_keyboard_interrupts == 0);
#endif

	/* Enable mouse */
	drop_mouse_interrupts = 1;
	ps2_send_ctrl(0xd4);
	ps2_send_data(0xf4);
	HALT_ON_ERRORCOND(ps2_recv_data() == 0xfa);
	HALT_ON_ERRORCOND(drop_mouse_interrupts == 0);

	printf("Initialized mouse\n");
}

void handle_mouse_interrupt(VCPU *vcpu, int vector, int guest)
{
	HALT_ON_ERRORCOND(vcpu->isbsp);
	HALT_ON_ERRORCOND(vector == 0x2c);
	if (drop_mouse_interrupts) {
		drop_mouse_interrupts--;
		printf("Dropped a mouse interrupt\n");
	} else {
		uint8_t scancode = inb(PS2_DATA_PORT);
		printf("CPU(0x%02x): mouse: 0x%hh02x, guest=%d\n", vcpu->id,
			   scancode, guest);
	}
	outb(INT_ACK_CURRENT, SLAVE_ICW);
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

