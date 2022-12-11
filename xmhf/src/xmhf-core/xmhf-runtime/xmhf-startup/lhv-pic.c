#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

void pic_init(void)
{
	/* Bring the master IDT up; yes I know that the IO looks odd.
	 * It's supposed to be, apparently.
	 */
	outb(PICM_ICW1, MASTER_ICW);
	outb(X86_PIC_MASTER_IRQ_BASE, MASTER_OCW);
	outb(PICM_ICW3, MASTER_OCW);
	outb(PICM_ICW4, MASTER_OCW);

	/* Same dance with the slave as with the master */
	outb(PICS_ICW1, SLAVE_ICW);
	outb(X86_PIC_SLAVE_IRQ_BASE, SLAVE_OCW);
	outb(PICS_ICW3, SLAVE_OCW);
	outb(PICS_ICW4, SLAVE_OCW);

	/* Tell the master and slave that any IRQs they had outstanding
	 * have been acknowledged.
	 */
	outb(NON_SPEC_EOI, MASTER_ICW);
	outb(NON_SPEC_EOI, SLAVE_ICW);

	/* Enable all IRQs on master and slave */
	outb (0, SLAVE_OCW);
	outb (0, MASTER_OCW);
}

/*
 * Return 1 if spurious, 0 if not, -1 if irq is wrong.
 */
int pic_spurious(unsigned char irq)
{
	if (irq <= 7) {
		outb(READ_IS_ONRD | OCW_TEMPLATE | READ_NEXT_RD, MASTER_ICW);
		return (inb(MASTER_ICW) & (1 << irq)) == 0;
	} else if (irq <= 15) {
		outb(READ_IS_ONRD | OCW_TEMPLATE | READ_NEXT_RD, SLAVE_ICW);
		return (inb(SLAVE_ICW) & (1 << (irq - 8))) == 0;
	} else {
		return -1;
	}
}
