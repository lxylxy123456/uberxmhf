#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define RTC_SECS  0
#define RTC_MINS  2
#define RTC_HOURS 4
#define RTC_DAY   7
#define RTC_MONTH 8
#define RTC_YEAR  9

#define RTC_PORT_OUT 0x70
#define RTC_PORT_IN  0x71

#define TIMER_PERIOD 50
#define TIMER_RATE 1193182
#define TIMER_PERIOD_IO_PORT 0x40
#define TIMER_MODE_IO_PORT 0x43
#define TIMER_SQUARE_WAVE 0x36

#define LAPIC_PERIOD (TIMER_PERIOD * 1000000)

static int getrtcfield(int field) {
	int bcd;
	outb(field, RTC_PORT_OUT);
	bcd = inb(RTC_PORT_IN);
	return (bcd & 0xF) + (bcd >> 4) * 10;
}

static int rtc_get_sec_of_day(void) {
	int h, h_new, m, m_new, s;
	h = getrtcfield(RTC_HOURS);
	while (1) {
		m = getrtcfield(RTC_MINS);
		while (1) {
			s = getrtcfield(RTC_SECS);
			m_new = getrtcfield(RTC_MINS);
			if (m != m_new) {
				m = m_new;
			} else {
				break;
			}
		}
		h_new = getrtcfield(RTC_HOURS);
		if (h != h_new) {
			h = h_new;
		} else {
			break;
		}
	}
	return h * 3600 + m * 60 + s;
}

void timer_init(VCPU *vcpu)
{
	/* PIT */
	if (vcpu->isbsp) {
		u64 ncycles = TIMER_RATE * TIMER_PERIOD / 1000;
		HALT_ON_ERRORCOND(ncycles == (u64)(u16)ncycles);
		outb(TIMER_SQUARE_WAVE, TIMER_MODE_IO_PORT);
		outb((u8)(ncycles), TIMER_PERIOD_IO_PORT);
		outb((u8)(ncycles >> 8), TIMER_PERIOD_IO_PORT);
	}

//	/* LAPIC Timer */
//	write_lapic(LAPIC_TIMER_DIV, 0x0000000b);
//	write_lapic(LAPIC_TIMER_INIT, LAPIC_PERIOD);
//	write_lapic(LAPIC_LVT_TIMER, 0x00020022);
}

static void update_screen(VCPU *vcpu, int *x, int y, int guest)
{
	console_vc_t vc;
	console_get_vc(&vc, vcpu->idx, guest);
	(*x) += vc.width;
	(*x) %= vc.width;
	if (!"update digit") {
		char c = console_get_char(&vc, *x, y);
		c -= '0';
		c++;
		c %= 10;
		c += '0';
		console_put_char(&vc, *x, y, c);
	} else {
		/* erase half point */
		console_put_char(&vc, *x, y, '0' + vcpu->idx);
		console_put_char(&vc, ((*x) + vc.width / 2) % vc.width, y, ' ');
	}
	(*x)++;
	*x %= vc.width;
}

static void calibrate_timer(VCPU *vcpu) {
	u64 p_t = vcpu->pit_time;
	u64 l_t = vcpu->lapic_time;
	u64 l_quo = read_lapic(LAPIC_TIMER_CUR);
	u64 l_tot = l_t * LAPIC_PERIOD + l_quo;
	/*
	 * y = a * x + b, x
	 * y / x = a + b / x > a
	 * (y - period) / x = a - (period - b) / x < a
	 */
	u64 cal_1 = (l_tot - LAPIC_PERIOD) / p_t;
	u64 cal_2 = l_tot / p_t;
	//printf("%lld %lld 0x%08llx 0x%08llx 0x%08llx - 0x%08llx %d\n",
	printf("%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%d\n",
			p_t, l_t, l_quo, l_tot, cal_1, cal_2, rtc_get_sec_of_day());
}

void handle_timer_interrupt(VCPU *vcpu, int vector, int guest)
{
	if (vector == 0x20) {
		vcpu->pit_time++;
		update_screen(vcpu, &vcpu->lhv_pit_x[guest], 0, guest);
		outb(INT_ACK_CURRENT, INT_CTL_PORT);
		if (!"Calibrate timer" && vcpu->pit_time % 50 == 0) {
			calibrate_timer(vcpu);
		}
	} else if (vector == 0x22) {
		vcpu->lapic_time++;
		update_screen(vcpu, &vcpu->lhv_lapic_x[guest], 1, guest);
		write_lapic(LAPIC_EOI, 0);
		if (!"Calibrate timer" && vcpu->isbsp && vcpu->lapic_time % 50 == 0) {
			calibrate_timer(vcpu);
		}
	} else {
		HALT_ON_ERRORCOND(0);
	}
}

