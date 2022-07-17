#include <xmhf.h>
#include <lhv.h>

static void lhv_exploit(VCPU *vcpu)
{
	uintptr_t apic_page = PA_PAGE_ALIGN_4K(rdmsr64(MSR_APIC_BASE));
	volatile u32 *icr_low = (volatile u32 *)(apic_page | 0x300UL);
	volatile u32 *icr_high = (volatile u32 *)(apic_page | 0x310UL);
	u32 i;

	/* Synchronize */
	{
		static volatile u32 ap_ready = 0;
		if (vcpu->isbsp) {
			while (!ap_ready) {
				asm volatile ("pause");		/* Save energy when waiting */
			}
			printf("BSP synchronized\n");
		} else {
			printf("AP synchronized\n");
			ap_ready = 1;
			HALT();
		}
	}

	HALT_ON_ERRORCOND(vcpu->isbsp);

	/* Prepare AP's real mode code */
	{
		*(volatile u8 *)0x10000 = 0x90;
		*(volatile u8 *)0x10001 = 0xeb;
		*(volatile u8 *)0x10002 = 0xfd;
	}

	/* Send INIT to AP */
	{
		*icr_high = 0x01000000U;
		*icr_low = 0x00004500U;
		printf("INIT\n");
		xmhf_baseplatform_arch_x86_udelay(10000);
		while ((*icr_low) & 0x1000U) {
			asm volatile ("pause");     /* Save energy when waiting */
		}
	}

	/* Send another INIT to AP */
	{
		*icr_high = 0x01000000U;
		*icr_low = 0x00004500U;
		printf("INIT\n");
		xmhf_baseplatform_arch_x86_udelay(10000);
		while ((*icr_low) & 0x1000U) {
			asm volatile ("pause");     /* Save energy when waiting */
		}
	}

	/* Send SIPI to AP */
	for (i = 0; i < 2; i++) {
		*icr_high = 0x01000000U;
		*icr_low = 0x00004610U;
		printf("SIPI\n");
		if (i) {
			break;
		}
		xmhf_baseplatform_arch_x86_udelay(10000);
		while ((*icr_low) & 0x1000U) {
			asm volatile ("pause");     /* Save energy when waiting */
		}
	}

	while (1) {
		asm volatile ("hlt");
	}
}

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
	/* Demonstrate disabling paging in hypervisor */
	if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
#ifdef __AMD64__
		extern void lhv_disable_enable_paging(char *);
		lhv_disable_enable_paging("LHV hypervisor can disable paging\n");
#elif defined(__I386__)
		ulong_t cr0 = read_cr0();
		write_cr0(cr0 & 0x7fffffffUL);
		printf("LHV hypervisor can disable paging\n");
		write_cr0(cr0);
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		/* Set EFLAGS.IF */
		asm volatile ("sti");
	}

	switch (vcpu->idx) {
	case 0:
		HALT_ON_ERRORCOND(vcpu->isbsp);
		printf("CPU(0x%02x): BSP\n", vcpu->id);
		break;
	case 1:
		printf("CPU(0x%02x): AP\n", vcpu->id);
		break;
	default:
		printf("CPU(0x%02x): halting\n", vcpu->id);
		HALT();
		break;
	}

	lhv_exploit(vcpu);

	/* Enter user mode (e.g. test TrustVisor) */
	if (__LHV_OPT__ & LHV_USER_MODE) {
		enter_user_mode(vcpu, 0);
	}

	HALT();
}

void handle_lhv_syscall(VCPU *vcpu, int vector, struct regs *r)
{
	/* Currently the only syscall is to exit guest mode */
	HALT_ON_ERRORCOND(vector == 0x23);
	HALT_ON_ERRORCOND(r->eax == 0xdeaddead);
	(void) vcpu;
	HALT();
}

