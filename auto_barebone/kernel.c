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

	{
		extern void emhfc_putchar(char ch);
		emhfc_putchar('H');
		emhfc_putchar('e');
		emhfc_putchar('l');
		emhfc_putchar('\n');
	}

	//printf("Hello, kernel World!\n", "asdf");

	cpu_halt();
}
