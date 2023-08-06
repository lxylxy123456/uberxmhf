/* Template from https://wiki.osdev.org/Bare_Bones */

#include <xmhf.h>

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

/* This function is called from boot.s, only BSP. */
void kernel_main(void)
{
	/* Initialize terminal interface */
	{
		extern void dbg_x86_vgamem_init(void);
		dbg_x86_vgamem_init();
	}

	/* initialize SMP */
	{
		smp_init();
	}

	HALT_ON_ERRORCOND(0 && "Should not reach here");
}

/* This function is called from smp-asm.S, both BSP and AP. */
void kernel_main_smp(VCPU *vcpu)
{
	printf("Hello, %s World %d!\n", "smp", vcpu->id);

	cpu_halt();
}
