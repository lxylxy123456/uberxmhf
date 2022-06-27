
.global enter_user_mode_asm
enter_user_mode_asm:
//	movw	$0x13, %ax
//	movw	%ax, %ds
//	movw	%ax, %es
//	movw	%ax, %fs
//	movw	%ax, %gs
	movl	4(%esp), %esp
	popal
	iret

