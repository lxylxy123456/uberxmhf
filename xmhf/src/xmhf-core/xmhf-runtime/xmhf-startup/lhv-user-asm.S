#include <xmhf.h>

.global enter_user_mode_asm
enter_user_mode_asm:
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movl	4(%esp), %esp
	popal
	iret

