#include <xmhf.h>

.global enter_user_mode_asm
enter_user_mode_asm:
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es
#ifdef __AMD64__
	movq	%rdi, %rsp
	POPAQ
	iretq
#elif defined(__I386__)
	movl	4(%esp), %esp
	popal
	iret
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

