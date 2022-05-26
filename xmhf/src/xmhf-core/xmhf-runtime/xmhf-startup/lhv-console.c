#include <xmhf.h>
#include <lhv.h>

#define CRTC_IDX_REG 0x3d4
#define CRTC_DATA_REG 0x3d5
#define CRTC_CURSOR_LSB_IDX 15
#define CRTC_CURSOR_MSB_IDX 14
#define CONSOLE_MEM_BASE ((volatile char *) 0xB8000)
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25

#define CONSOLE_MAX_CPU 8

void console_cursor_clear(void)
{
	outb(CRTC_CURSOR_LSB_IDX, CRTC_IDX_REG);
	outb(0, CRTC_DATA_REG);
	outb(CRTC_CURSOR_MSB_IDX, CRTC_IDX_REG);
	outb(0, CRTC_DATA_REG);
}

static volatile char *console_get_mmio(console_vc_t *vc, int x, int y)
{
	if (!vc) {
		HALT_ON_ERRORCOND(0 <= x && x < CONSOLE_WIDTH);
		HALT_ON_ERRORCOND(0 <= y && y < CONSOLE_HEIGHT);
		return CONSOLE_MEM_BASE + 2 * (x + CONSOLE_WIDTH * y);
	}
	HALT_ON_ERRORCOND(0 <= x && x < vc->width);
	HALT_ON_ERRORCOND(0 <= y && y < vc->height);
	return console_get_mmio(NULL, x + vc->left, y + vc->top);
}

void console_clear(console_vc_t *vc)
{
	for (int i = 0; i < vc->width; i++) {
		for (int j = 0; j < vc->height; j++) {
			volatile char *p = console_get_mmio(vc, i, j);
			p[0] = ' ';
			p[1] = vc->color;
		}
	}
}

char console_get_char(console_vc_t *vc, int x, int y)
{
	volatile char *p = console_get_mmio(vc, x, y);
	return p[0];
}

void console_put_char(console_vc_t *vc, int x, int y, char c)
{
	volatile char *p = console_get_mmio(vc, x, y);
	p[0] = c;
}

void console_get_vc(console_vc_t *vc, int num, int guest)
{
	HALT_ON_ERRORCOND(0 <= num && num < CONSOLE_MAX_CPU);
	HALT_ON_ERRORCOND(0 <= guest && guest < 2);
	vc->left = (CONSOLE_WIDTH / 2) * guest;
	vc->top = (CONSOLE_HEIGHT / CONSOLE_MAX_CPU) * num;
	vc->width = (CONSOLE_WIDTH / 2);
	vc->height = (CONSOLE_HEIGHT / CONSOLE_MAX_CPU);
	/* Use 9 - 15 */
	vc->color = (char) ((1 + num * 2 + guest) % 7 + 9);
}

