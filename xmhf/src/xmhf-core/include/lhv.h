#include <xmhf.h>

#ifndef _LHV_H_
#define _LHV_H_

typedef struct {
	int left;
	int top;
	int width;
	int height;
	char color;
} console_vc_t;

/* xcph-x86.c */
VCPU *_svm_and_vmx_getvcpu(void);

/* lhv-console.c */
void console_cursor_clear(void);
void console_clear(console_vc_t *vc);
char console_get_char(console_vc_t *vc, int x, int y);
void console_put_char(console_vc_t *vc, int x, int y, char c);
void console_get_vc(console_vc_t *vc, int num);

/* lhv-timer.c */
void timer_init(void);
void handle_timer_interrupt(VCPU *vcpu, int vector);

/* lhv-pic.c */
void pic_init(void);

/* lhv-keyboard.c */
void handle_keyboard_interrupt(VCPU *vcpu, int vector);

/* LAPIC */
#define LAPIC_DEFAULT_BASE    0xfee00000
#define IOAPIC_DEFAULT_BASE   0xfec00000
#define LAPIC_LVT_TIMER        0x320     /* Local Vector Table 0 (TIMER) */
#define LAPIC_TIMER_INIT       0x380     /* Timer Initial Count */
#define LAPIC_TIMER_CUR        0x390     /* Timer Current Count */
#define LAPIC_TIMER_DIV        0x3E0     /* Timer Divide Configuration */

static inline u32 read_lapic(u32 reg) {
	return *(volatile u32 *)(uintptr_t)(LAPIC_DEFAULT_BASE + reg);
}

static inline void write_lapic(u32 reg, u32 val) {
	*(volatile u32 *)(uintptr_t)(LAPIC_DEFAULT_BASE + reg) = val;
}

#endif /* _LHV_H_ */
