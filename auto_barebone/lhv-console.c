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
	outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
	outb(CRTC_DATA_REG, 0);
	outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
	outb(CRTC_DATA_REG, 0);
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

