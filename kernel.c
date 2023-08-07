/* Template from https://wiki.osdev.org/Bare_Bones */

#include <xmhf.h>

/* This function is called from boot.s, only BSP. */
void kernel_main(void)
{
	/* Initialize terminal interface. */
	{
		extern void dbg_x86_vgamem_init(void);
		dbg_x86_vgamem_init();
	}

	/* Set up page table and enable paging. */
	{
		uintptr_t cr0 = read_cr0();
		uintptr_t cr3 = shv_page_table_init();
		write_cr3(cr3);
		if ((cr0 & CR0_PG) == 0) {
			// TODO: write_cr0(cr0 | CR0_PG);
		}
	}

	/* Initialize SMP. */
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
