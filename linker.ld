/* From https://wiki.osdev.org/Bare_Bones and XMHF runtime.lds.S */

/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

MEMORY
{
	all (rwxai) : ORIGIN = 1M, LENGTH = 4095M
	debug (rwxai) : ORIGIN = 0, LENGTH = 1024M
	unaccounted (rwxai) : ORIGIN = 0, LENGTH = 0 /* see .unaccounted at end */
}

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 1M;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text BLOCK(4K) : ALIGN(4K)
	{
		KEEP(*(.multiboot))
		*(.multiboot)
		*(.text)
		*(.text.*)
		*(.rel.text)
		*(.rel.text.*)
	}

	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
		*(.rodata.*)
		*(.rel.rodata)
		*(.rel.rodata.*)
		*(.comment)
		*(.eh_frame) /* exception-metadata. might be able to discard */
		. = ALIGN(8);
		_begin_xcph_table = .;
		KEEP(*(.xcph_table))
		*(.xcph_table)
		_end_xcph_table = .;
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
		*(.data.*)
		*(.rel.data)
		*(.rel.data.*)
		*(.got)
		*(.got.plt)
		. = ALIGN(16);
		_end_multiboot_data = .;
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
		*(SORT_BY_ALIGNMENT(.bss.*))
		*(.rel.bss)
		*(.rel.bss.*)
		*(.note.gnu.build-id)
		. = ALIGN(16);
		_end_multiboot_bss = .;
	}

	/* debug sections */
	.debug_abbrev : { *(.debug_abbrev) } >debug
	.debug_aranges : { *(.debug_aranges) } >debug
	.debug_info : { *(.debug_info) } >debug
	.debug_line : { *(.debug_line) } >debug
	.debug_line_str : { *(.debug_line_str) } >debug
	.debug_loc : { *(.debug_loc) } >debug
	.debug_loclists : { *(.debug_loclists) } >debug
	.debug_ranges : { *(.debug_ranges) } >debug
	.debug_rnglists : { *(.debug_rnglists) } >debug
	.debug_str : { *(.debug_str) } >debug
	.rel.debug_info : { *(.rel.debug_info) } >debug
	.rel.debug_aranges : { *(.rel.debug_aranges) } >debug
	.rel.debug_line : { *(.rel.debug_line) } >debug

	/* this is to cause the link to fail if there is
	* anything we didn't explicitly place.
	* when this does cause link to fail, temporarily comment
	* this part out to see what sections end up in the output
	* which are not handled above, and handle them.
	*/
	.unaccounted : {
		*(*)
	} >unaccounted
}
