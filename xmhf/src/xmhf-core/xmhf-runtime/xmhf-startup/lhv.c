#include <xmhf.h>
#include <lhv.h>

void lhv_main(VCPU *vcpu)
{
	console_vc_t vc;
	console_get_vc(&vc, vcpu->idx);
	console_clear(&vc);
	if (vcpu->isbsp) {
		console_cursor_clear();
		timer_init();
		// asm volatile ("int $0xf8");
		if (0) {
			int *a = (int *) 0xf0f0f0f0f0f0f0f0;
			printf("%d", *a);
		}
	}
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < vc.height; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	if (vcpu->isbsp) {
		if (1) {
			uintptr_t a;
			get_eflags(a);
			a |= EFLAGS_IF;
			set_eflags(a);
		}
	}
	if (vcpu->isbsp) {
		printf("\nLVT: 0x%08x", read_lapic(LAPIC_LVT_TIMER));
		printf("\nINI: 0x%08x", read_lapic(LAPIC_TIMER_INIT));
		printf("\nCUR: 0x%08x", read_lapic(LAPIC_TIMER_CUR));
		printf("\nDIV: 0x%08x", read_lapic(LAPIC_TIMER_DIV));
	}

	HALT();
}

