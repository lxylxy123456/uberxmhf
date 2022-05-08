#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define TIMER_PERIOD 50
#define TIMER_RATE 1193182
#define TIMER_PERIOD_IO_PORT 0x40
#define TIMER_MODE_IO_PORT 0x43
#define TIMER_SQUARE_WAVE 0x36

void timer_init(void)
{
	/* PIT */
	u64 ncycles = TIMER_RATE * TIMER_PERIOD / 1000;
	HALT_ON_ERRORCOND(ncycles == (u64)(u16)ncycles);
	outb(TIMER_SQUARE_WAVE, TIMER_MODE_IO_PORT);
	outb((u8)(ncycles), TIMER_PERIOD_IO_PORT);
	outb((u8)(ncycles >> 8), TIMER_PERIOD_IO_PORT);

	/* LAPIC Timer */
	write_lapic(LAPIC_TIMER_DIV, 0x00000003);
	write_lapic(LAPIC_TIMER_INIT, 0x00003000);
	write_lapic(LAPIC_LVT_TIMER, 0x00020022);
}

void handle_timer_interrupt(VCPU *vcpu, int vector)
{
	static int x = 0;
	static int y = 0;
	console_vc_t vc;
	console_get_vc(&vc, vcpu->idx);
	(void) vector;
	{
		char c = console_get_char(&vc, x, y);
		c -= '0';
		c++;
		c %= 10;
		c += '0';
		console_put_char(&vc, x, y, c);
	}
	x++;
	y += x / vc.width;
	x %= vc.width;
	y %= vc.height;
	outb(INT_ACK_CURRENT, INT_CTL_PORT);
}

