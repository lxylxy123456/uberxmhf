
.global enter_user_mode_asm
enter_user_mode_asm:
	movw	$0x33, %ax
	movw	%ax, %ds
	movl	4(%esp), %esp
	popal
	iret

