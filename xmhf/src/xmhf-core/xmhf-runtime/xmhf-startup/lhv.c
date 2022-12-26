#include <xmhf.h>
#include <lhv.h>

#ifndef __AMD64__
#error "Only AMD64 supported"
#endif

u64 lxy_key;

void check_page(u64 pa) {
	if (pa < 0x1000000) {
		return;
	}
	if (pa % 0x1000000 != 0) {
		return;
	}

	{
		unsigned char *p = (unsigned char *)(uintptr_t)pa;
		printf("  0x%016llx: %08x %08x %016llx\n", pa, *(u32 *)(p + 0), *(u32 *)(p + 4), *(u64 *)(p + 8));
		p[0] = 0x58;
		p[1] = 0x4d;
		p[2] = 0x48;
		p[3] = 0x46;
		*(u32 *)(p + 4) = pa >> 24;
		*(u64 *)(p + 8) = lxy_key;
	}
}

void exploit(VCPU *vcpu) {
	if (!vcpu->isbsp) {
		HALT();
		return;
	}

	printf("Number of E820 entries = %u\n", rpb->XtVmmE820NumEntries);
	lxy_key = rdtsc64();

	for(int i=0; i < (int)rpb->XtVmmE820NumEntries; i++){
		u64 baseaddr = (((u64)g_e820map[i].baseaddr_high << 32) |
						g_e820map[i].baseaddr_low);
		u64 length = (((u64)g_e820map[i].length_high << 32) |
					  g_e820map[i].length_low);
		if (g_e820map[i].type != 1) {
			continue;
		}
		while (baseaddr % PA_PAGE_SIZE_4K != 0 && length > 0) {
			baseaddr += 1;
			length -= 1;
		}
		length = PA_PAGE_ALIGN_4K(length);
		printf("Available: 0x%016llx, 0x%016llx\n", baseaddr, length);
		for (u64 j = baseaddr; j < baseaddr + length; j += PA_PAGE_SIZE_4K) {
			check_page(j);
		}
	}

	wbinvd();
	printf("WBINVD complete, lxy_key = 0x%016llx\n", lxy_key);

	for (int i = 0; i < 0x10000000; i++) {
		asm volatile ("pause");
	}

	printf("Restarting");
	asm volatile ("pushq $0; pushq $0; lidt (%rsp); ud2;");

	HALT();
}

void lhv_main(VCPU *vcpu)
{
	console_vc_t vc;
	console_get_vc(&vc, vcpu->idx, 0);
	console_clear(&vc);
	exploit(vcpu);
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

	/* Start VT related things */
	lhv_vmx_main(vcpu);

	HALT();
}

