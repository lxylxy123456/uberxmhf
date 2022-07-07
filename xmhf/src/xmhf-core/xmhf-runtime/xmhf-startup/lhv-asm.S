#include <xmhf.h>
#include <lhv.h>

.global vmexit_asm
.global vmlaunch_asm
.global vmresume_asm

vmexit_asm:
#ifdef __XMHF_AMD64__
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	pushq	%rbx
	pushq	%rsp
	pushq	%rbp
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movq	128(%rsp), %rdi
	movq	%rsp, %rsi

	/* Optional code to check EFLAGS */
	pushfq
	popq	%rax
9:	cmpq	$2, %rax
	jne		9b
#elif defined(__I386__)
	pushal
	movl	32(%esp), %esi
	movl	%esp, %eax
	pushl	%eax
	pushl	%esi

	/* Optional code to check EFLAGS */
	pushfl
	popl	%eax
9:	cmpl	$2, %eax
	jne		9b
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	call	vmexit_handler
1:	jmp 1b				/* Should never return */

.altmacro
.macro vmentry_asm_template inst_name inst_num

inst_name&_asm:
#ifdef __XMHF_AMD64__
	movq	%rdi, %rsp
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdi
	popq	%rsi
	popq	%rbp
	leaq	8(%rsp), %rsp
	popq	%rbx
	popq	%rdx
	popq	%rcx
	popq	%rax
#elif defined(__I386__)
	movl	4(%esp), %esp
	popal
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	inst_name
	je		2f			/* ZF set means VMfailValid */
	jb		3f			/* CF set means VMfailInvalid */
1:	jmp		1b			/* Unknown error */
2:
#ifdef __XMHF_AMD64__
	movq	$1, %rsi
#elif defined(__I386__)
	pushl	$1
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	jmp		4f
3:
#ifdef __XMHF_AMD64__
	movq	$0, %rsi
#elif defined(__I386__)
	pushl	$0
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	jmp		4f
4:
#ifdef __XMHF_AMD64__
	movq	$&inst_num&, %rdi	/* vmlaunch = 0, vmresume = 1 */
#elif defined(__I386__)
	pushl	$&inst_num&			/* vmlaunch = 0, vmresume = 1 */
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	call	vmentry_error
1:	jmp		1b			/* Should never return */

.endm

vmentry_asm_template vmlaunch 0
vmentry_asm_template vmresume 1

#ifdef __AMD64__
.global lhv_disable_enable_paging
lhv_disable_enable_paging:
	/* Argument %rdi is pointer to string to print */
	movq	%rdi, %rcx

	/* Jump to compatibility mode */
	pushq	$(__CS32)
	pushq	$1f
	lretq
1:
.code32

	/* Disable paging */
	movl	%cr0, %eax
	andl	$0x7fffffff, %eax
	movl	%eax, %cr0

	/* Get printf lock */
	movl	(emhfc_putchar_linelock_arg), %eax
1:	pause
	btl		$0, (%eax)
	jnc		1b
	lock
	btrl	$0, (%eax)
	jnc		1b

	/* Print the string at %ecx */
2:	cmpb	$0, (%ecx)
	je		3f

	/* Use inb to make sure the serial port is ready */
	movw	$(0x3f8 + 5), %dx
1:	inb		(%dx), %al
	testb	$0x20, %al
	jz		1b

	/* Use outb to print to serial port */
	movw	$(0x3f8), %dx
	movb	(%ecx), %al
	outb	%al, (%dx)
	incl	%ecx

	jmp		2b
3:

	/* Release printf lock */
	movl	(emhfc_putchar_linelock_arg), %eax
	btsl	$0, (%eax)

	/* Enable paging */
	movl	%cr0, %eax
	orl		$0x80000000, %eax
	movl	%eax, %cr0

	/* Jump to long mode */
	pushl	$(__CS)
	pushl	$1f
	lretl
1:
.code64
	/* Done */
	ret
#endif /* __AMD64__ */

