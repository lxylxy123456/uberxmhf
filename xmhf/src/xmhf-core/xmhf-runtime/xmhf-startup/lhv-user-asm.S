#include <xmhf.h>

.global enter_user_mode_asm
enter_user_mode_asm:
	/* Prologue */
#ifdef __AMD64__
	pushq	%rbp
	movq	%rsp, %rbp
#elif defined(__I386__)
	pushl	%ebp
	movl	%esp, %ebp
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	/* Save kernel mode state */
#ifdef __AMD64__
	PUSHAQ
	pushfq
	movq	%rsp, (%rsi)
#elif defined(__I386__)
	pushal
	pushfl
	movl	12(%ebp), %eax
	movl	%esp, (%eax)
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	/* Set user DS and ES */
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* IRET to user mode */
#ifdef __AMD64__
	movq	%rdi, %rsp
	POPAQ
	iretq
#elif defined(__I386__)
	movl	8(%ebp), %esp
	popal
	iret
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

.global exit_user_mode_asm
exit_user_mode_asm:

	/* Restore kernel mode state */
#ifdef __AMD64__
	movq	%rdi, %rsp
	popfq
	POPAQ
#elif defined(__I386__)
	movl	4(%esp), %esp
	popfl
	popal
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	/* Epilogue */
#ifdef __AMD64__
	popq	%rbp
#elif defined(__I386__)
	popl	%ebp
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	ret

