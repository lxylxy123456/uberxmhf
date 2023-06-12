/* Template from https://wiki.osdev.org/Bare_Bones */

#include <xmhf.h>

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main(void)
{
	/* Initialize terminal interface */
	{
		extern void dbg_x86_vgamem_init(void);
		dbg_x86_vgamem_init();
	}

//	terminal_writestring("Hello, kernel World!\n");

	printf("Hello, %s World %d!\n", "kernel", 1);

	cpu_halt();
}
