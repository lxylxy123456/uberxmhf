#include <xmhf.h>
#include <lhv.h>

#define TIMER_PERIOD 10
#define TIMER_RATE 1193182
#define TIMER_PERIOD_IO_PORT 0x40
#define TIMER_MODE_IO_PORT 0x43
#define TIMER_SQUARE_WAVE 0x36

void timer_init(void)
{
	u64 ncycles = TIMER_RATE * TIMER_PERIOD / 1000;
	HALT_ON_ERRORCOND(ncycles == (u64)(u16)ncycles);
	outb(TIMER_SQUARE_WAVE, TIMER_MODE_IO_PORT);
	outb((u8)(ncycles), TIMER_PERIOD_IO_PORT);
	outb((u8)(ncycles >> 8), TIMER_PERIOD_IO_PORT);
}

