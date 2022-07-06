#include <xmhf.h>
#include <lhv.h>

static inline void write_cr0_2(unsigned long val){
	__asm__("mov $0x4321, %%eax; vmcall; mov %0,%%cr0": :"b" ((unsigned long)val));
}

void lhv_guest_main(ulong_t cpu_id)
{
	ulong_t cr0 = read_cr0();
	write_cr0(cr0 & 0x7fffffffUL);
	printf("CPU(0x%02x): LHV guest can disable paging\n", cpu_id);
	write_cr0_2(cr0);
	printf("CPU(0x%02x): LHV guest can enable paging\n", cpu_id);
	HALT();
}

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	(void) r;
	HALT_ON_ERRORCOND(0 && "Guest received exception (incorrect behavior)");
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
