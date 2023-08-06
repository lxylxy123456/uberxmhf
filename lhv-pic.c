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

void pic_init(void)
{
	/* Bring the master IDT up; yes I know that the IO looks odd.
	 * It's supposed to be, apparently.
	 */
	outb(MASTER_ICW, PICM_ICW1);
	outb(MASTER_OCW, X86_PIC_MASTER_IRQ_BASE);
	outb(MASTER_OCW, PICM_ICW3);
	outb(MASTER_OCW, PICM_ICW4);

	/* Same dance with the slave as with the master */
	outb(SLAVE_ICW, PICS_ICW1);
	outb(SLAVE_OCW, X86_PIC_SLAVE_IRQ_BASE);
	outb(SLAVE_OCW, PICS_ICW3);
	outb(SLAVE_OCW, PICS_ICW4);

	/* Tell the master and slave that any IRQs they had outstanding
	 * have been acknowledged.
	 */
	outb(MASTER_ICW, NON_SPEC_EOI);
	outb(SLAVE_ICW, NON_SPEC_EOI);

	/* Enable all IRQs on master and slave */
	outb(SLAVE_OCW, 0);
	outb(MASTER_OCW, 0);
}

/*
 * Return 1 if spurious, 0 if not, -1 if irq is wrong.
 */
int pic_spurious(unsigned char irq)
{
	if (irq <= 7) {
		outb(MASTER_ICW, READ_IS_ONRD | OCW_TEMPLATE | READ_NEXT_RD);
		return (inb(MASTER_ICW) & (1 << irq)) == 0;
	} else if (irq <= 15) {
		outb(SLAVE_ICW, READ_IS_ONRD | OCW_TEMPLATE | READ_NEXT_RD);
		return (inb(SLAVE_ICW) & (1 << (irq - 8))) == 0;
	} else {
		return -1;
	}
}
