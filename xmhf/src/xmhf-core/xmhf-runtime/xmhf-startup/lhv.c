#include <xmhf.h>
#include <lhv.h>

//#define MOD 100000
#define MOD 1
volatile uintptr_t val = 0;
//volatile uintptr_t *ptr = &val;
volatile uintptr_t *ptr = (volatile uintptr_t *) 0xb8000;
volatile bool flag = false;
u64 count0 = 0;
u64 count1 = 0;
u64 count2 = 0;

/*

Convert to other hypervisors' formats
 qemu-img convert -f raw -O vmdk grub/c.img grub/c.vmdk
 qemu-img convert -f raw -O vhdx grub/c.img grub/c.vhdx

*/

void lhv_main(VCPU *vcpu)
{
	{
		static bool ready = false;
		if (vcpu->isbsp) {
			mb();
			*ptr = 0;
			mb();
			ready = true;
			mb();
		} else {
			while (!ready) {
				xmhf_cpu_relax();
			}
		}
	}
	switch (vcpu->idx) {
	case 0:
		printf("counts:\tcount0\tcount1\tcount2\tcount0-count1+count2\n");
		while (1) {
			uintptr_t ax = 0x12345678U;
			flag = true;
			mb();
			asm volatile("lock xchg %0, %1" : "+r"(ax), "+m"(*ptr));
			mb();
			HALT_ON_ERRORCOND(ax == 0);
			count0++;
			mb();
			while (flag) {
				xmhf_cpu_relax();
			}
			mb();
			if (count0 % MOD == 0) {
				printf("counts:\t%llu\t%llu\t%llu\t%lld\n", count0, count1,
					   count2, count0 - (count1 + count2));
				if (count0 == (count1 + count2)) {
					(*(u8 *)0xb8030)++;
				} else {
					(*(u8 *)0xb80d0)++;
				}
			}
			mb();
		}
		break;
	case 1:
		while (1) {
			uintptr_t ax = 0;
			asm volatile("lock xchg %0, %1" : "+r"(ax), "+m"(*ptr));
			mb();
			if (ax) {
				mb();
				count1++;
				mb();
				flag = false;
				mb();
				(*(u8 *)0xb8050)++;
				mb();
			} else {
				xmhf_cpu_relax();
				(*(u8 *)0xb80f0)++;
			}
		}
		break;
	case 2:
		while (1) {
			uintptr_t ax = 0;
			asm volatile("lock xchg %0, %1" : "+r"(ax), "+m"(*ptr));
			mb();
			if (ax) {
				mb();
				count2++;
				mb();
				flag = false;
				mb();
				(*(u8 *)0xb8060)++;
				mb();
			} else {
				xmhf_cpu_relax();
				(*(u8 *)0xb8100)++;
			}
		}
		break;
	default:
		break;
	}
	HALT();
}

void lhv_main_old(VCPU *vcpu)
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

	/* Start VT related things */
	lhv_vmx_main(vcpu);

	HALT();
}

