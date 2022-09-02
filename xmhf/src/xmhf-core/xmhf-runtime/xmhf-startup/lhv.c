#include <xmhf.h>
#include <lhv.h>

extern volatile u32 xmhf_baseplatform_arch_x86_smpinitialize_commonstart_flag;

volatile u32 ap_restarted = 0;

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
			for (u32 i = 0; i < 100; i++) {
				xmhf_baseplatform_arch_x86_udelay(10000);
			}
		} else {
			printf("AP synchronized\n");
			ap_ready = 1;
			HALT();
		}
	}

	HALT_ON_ERRORCOND(vcpu->isbsp);

	printf("Prepare AP's real mode code\n");

	/* Prepare AP's real mode code */
	{
		uintptr_t end = (uintptr_t)&_ap_bootstrap_end;
		uintptr_t start = (uintptr_t)&_ap_bootstrap_start;
		xmhf_baseplatform_arch_x86_smpinitialize_commonstart_flag = 1;
		if (!"use simple real mode program") {
			extern void (*exploit_real_start)(void);
			extern void (*exploit_real_end)(void);
			end = (uintptr_t)&exploit_real_end;
			start = (uintptr_t)&exploit_real_start;
		}
		memcpy((void *)0x10000, (void *)start, end - start);
	}

	printf("Send INIT to AP\n");

	for (u32 i = 0; i < 1000; i++) {
		xmhf_baseplatform_arch_x86_udelay(10000);
	}

	/* Send INIT to AP */
	*icr_high = 0x01000000U;
	for (i = 0; i < 0x10000; i++) {
		*icr_low = 0x00004500U;
		while ((*icr_low) & 0x1000U) {
		}
	}

	{
		*icr_low = 0x00004500U;
		printf("INIT\n");
		xmhf_baseplatform_arch_x86_udelay(1);
		while ((*icr_low) & 0x1000U) {
			asm volatile ("pause");     /* Save energy when waiting */
		}
	}

	printf("Send INIT 2 to AP\n");

	for (u32 i = 0; i < 100; i++) {
		xmhf_baseplatform_arch_x86_udelay(10000);
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

	printf("Sent INIT 3+ to AP\n");

	for (u32 i = 0; i < 100; i++) {
		xmhf_baseplatform_arch_x86_udelay(10000);
	}

	/* Send SIPI to AP */
	for (i = 0; i < 2; i++) {
		*icr_high = 0x01000000U;
		*icr_low = 0x00004610U;
		printf("SIPI\n");
		if (i) {
			break;
		}
		xmhf_baseplatform_arch_x86_udelay(200);
		while ((*icr_low) & 0x1000U) {
			asm volatile ("pause");     /* Save energy when waiting */
		}
	}

	printf("Sent INIT to AP\n");

	for (u32 i = 0; i < 100; i++) {
		xmhf_baseplatform_arch_x86_udelay(10000);
	}

	/* Wait for AP to restart */
	{
		while (!ap_restarted) {
			asm volatile ("pause");		/* Save energy when waiting */
		}
		printf("BSP synchronized 2\n");
	}

	enter_user_mode(vcpu, 0);

	while (1) {
		asm volatile ("hlt");
	}
}

void lhv_exploit_vmxroot(VCPU *vcpu)
{
	if ("enable APIC timer") {
		write_lapic(LAPIC_SVR, read_lapic(LAPIC_SVR) | LAPIC_ENABLE);
		timer_init(vcpu);
		asm volatile ("sti");
	}
	{
		printf("AP synchronized 2\n");
		ap_restarted = 1;
	}
	for (u32 i = 0; i < 0x1000000; i++) {
		;
	}
	{
		extern u8 pal_demo_data[MAX_VCPU_ENTRIES][PAGE_SIZE_4K];
		while (1) {
			printf("data=0x%08lx\n", *(uintptr_t *)(pal_demo_data[0]));
			asm volatile ("hlt");
		}
	}
	while (1) {
		asm volatile ("hlt");
	}
}

void lhv_exploit_nmi_handler(VCPU *vcpu)
{
/* Use this shell script:
for i in g_vmx_{{{,lock_}quiesce,{,lock_}quiesce_resume}_counter,quiesce_resume_signal}; do
	nm xmhf/src/xmhf-core/xmhf-runtime/runtime.exe | grep $i
done
 */
	volatile u32 *p_quiesce_counter             = (volatile u32 *)0x1025e104UL;
	volatile u32 *p_lock_quiesce_counter        = (volatile u32 *)0x1025e108UL;
	volatile u32 *p_quiesce_resume_counter      = (volatile u32 *)0x1025e10cUL;
	volatile u32 *p_lock_quiesce_resume_counter = (volatile u32 *)0x1025e110UL;
	volatile u32 *p_quiesce_resume_signal       = (volatile u32 *)0x1025e11cUL;
	/* Copy the logic from xmhf_smpguest_arch_x86vmx_nmi_check_quiesce() */
	spin_lock(p_lock_quiesce_counter);
	(*p_quiesce_counter)++;
	spin_unlock(p_lock_quiesce_counter);
	while (!*p_quiesce_resume_signal) {
		asm volatile ("pause");		/* Save energy when waiting */
	}
	spin_lock(p_lock_quiesce_resume_counter);
	(*p_quiesce_resume_counter)++;
	spin_unlock(p_lock_quiesce_resume_counter);
	(void)vcpu;
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
#ifndef __DEBUG_VGA__
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
#endif /* !__DEBUG_VGA__ */
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

