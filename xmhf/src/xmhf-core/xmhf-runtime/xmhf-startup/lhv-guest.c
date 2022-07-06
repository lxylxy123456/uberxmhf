#include <xmhf.h>
#include <lhv.h>

static inline void write_cr0_2(unsigned long val){
	__asm__("mov $0x4321, %%eax; vmcall; mov %0,%%cr0": :"b" ((unsigned long)val));
}

void lhv_guest_main(ulong_t cpu_id)
{
	if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
		ulong_t cr0 = read_cr0();
		asm volatile ("cli");
		write_cr0(cr0 & 0x7fffffffUL);
		if (1) {
			printf("CPU(0x%02x): LHV guest can disable paging\n", cpu_id);
		}
		write_cr0_2(cr0);
		if (1) {
			printf("CPU(0x%02x): LHV guest can enable paging\n", cpu_id);
		}
		asm volatile ("sti");
	}
	HALT();
}

void lhv_guest_main2(ulong_t cpu_id)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	console_vc_t vc;
	HALT_ON_ERRORCOND(cpu_id == vcpu->idx);
	console_get_vc(&vc, vcpu->idx, 1);
	console_clear(&vc);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		asm volatile ("sti");
	}
	while (1) {
		if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
			asm volatile ("hlt");
		}
		asm volatile ("vmcall");
		if (__LHV_OPT__ & LHV_USE_EPT) {
			u32 a = 0xdeadbeef;
			u32 *p = (u32 *)0x12340000;
			asm volatile("movl (%1), %%eax" :
						 "+a" (a) :
						 "b" (p) :
						 "cc", "memory");
			if (0) {
				printf("CPU(0x%02x): EPT result: 0x%08x 0x%02x\n", vcpu->id, a,
					   vcpu->ept_num);
			}
			if (vcpu->ept_num == 0) {
				HALT_ON_ERRORCOND(a == 0xfee1c0de);
			} else {
				HALT_ON_ERRORCOND((u8) a == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 8) == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 16) == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 24) == vcpu->ept_num);
			}
		}
		if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
			ulong_t cr0 = read_cr0();
			asm volatile ("cli");
			write_cr0(cr0 & 0x7fffffffUL);
			if (0) {
				printf("CPU(0x%02x): LHV guest can disable paging\n", vcpu->id);
			}
			write_cr0(cr0);
			asm volatile ("sti");
		}
	}
}

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	(void) r;
	switch (vector) {
	case 0x20:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x22:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	default:
		printf("Guest: interrupt / exception vector %ld\n", vector);
		HALT_ON_ERRORCOND(0 && "Guest: unknown interrupt / exception!\n");
		break;
	}
}
