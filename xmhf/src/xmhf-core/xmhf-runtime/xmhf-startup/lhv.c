#include <xmhf.h>
#include <lhv.h>

void lhv_main(VCPU *vcpu)
{
	console_vc_t vc;
	console_get_vc(&vc, vcpu->idx, 0);
	console_clear(&vc);
	if (vcpu->isbsp) {
		console_cursor_clear();
		// asm volatile ("int $0xf8");
		if (0) {
			int *a = (int *) 0xf0f0f0f0f0f0f0f0;
			printf("%d", *a);
		}
	}
	timer_init(vcpu);
	assert(vc.height >= 2);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		/* Set EFLAGS.IF */
		uintptr_t a;
		get_eflags(a);
		a |= EFLAGS_IF;
		set_eflags(a);
	}

	/* Start VT related things */
	lhv_vmx_main(vcpu);

	HALT();
}

