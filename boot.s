/* From https://wiki.osdev.org/Bare_Bones */

/* Declare constants for the multiboot header. */
.set FLAGS,    0x10003          /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/*
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
.section .multiboot
multiboot_header:
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	.long multiboot_header
	.long multiboot_header
	.long _end_multiboot_data
	.long _end_multiboot_bss
	.long _start

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.code32
.global _start
.type _start, @function
_start:

#ifdef __amd64__

	/* Check support of CPUID.80000001H */
	movl	$0x80000000, %eax
	cpuid
	cmpl	$0x80000001, %eax
	ja		2f
1:	hlt
	jmp		1b
2:

	/* Check support of 64-bit mode: CPUID.80000001H:EDX.[bit 29] */
	movl	$0x80000001, %eax
	cpuid
	testl	$(1 << 29), %edx
	jne		2f
1:	hlt
	jmp		1b
2:

	/* Set up page table for 64-bit. */
	movl	$(shv_pdpt), %eax
	orl		$3, %eax
	movl	%eax, shv_pml4t

	hlt

.code64

#endif /* __amd64__ */

	/* Set up stack pointer. */
	movl	$stack_top, %esp

	/* Transfer control to C code. */
	call	kernel_main

	/* C code should not return. */
	cli
1:	hlt
	jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
