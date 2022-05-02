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
		if (1) {	// TODO: currently will trigger double fault
			uintptr_t a;
			get_eflags(a);
			a |= EFLAGS_IF;
			set_eflags(a);
		}
	}
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < vc.height; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	printf("\nCPU(0x%02x): not implemented", vcpu->id);
	HALT_ON_ERRORCOND(0);
}

