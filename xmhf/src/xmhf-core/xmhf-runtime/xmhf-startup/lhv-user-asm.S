#include <xmhf.h>

.global enter_user_mode_asm
enter_user_mode_asm:
	/* Save kernel mode state */
#ifdef __AMD64__
	PUSHAQ
	movq	%rsp, (%rsi)
#elif defined(__I386__)
	pushal
	movl	32+8(%esp), %eax
	movl	%esp, (%eax)
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	/* Set DS and ES */
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es
	/* IRET to user mode */
#ifdef __AMD64__
	movq	%rdi, %rsp
	POPAQ
	iretq
#elif defined(__I386__)
	movl	32+4(%esp), %esp
	popal
	iret
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

.global exit_user_mode_asm
exit_user_mode_asm:
#ifdef __AMD64__
	movq	%rdi, %rsp
	POPAQ
#elif defined(__I386__)
	movl	4(%esp), %esp
	popal
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	ret

