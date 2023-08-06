/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <xmhf.h>

.global enter_user_mode_asm
enter_user_mode_asm:
	/* Prologue */
#ifdef __amd64__
	pushq	%rbp
	movq	%rsp, %rbp
#elif defined(__i386__)
	pushl	%ebp
	movl	%esp, %ebp
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */

	/* Save kernel mode state */
#ifdef __amd64__
	PUSHAQ
	pushfq
	movq	%rsp, (%rsi)
#elif defined(__i386__)
	pushal
	pushfl
	movl	12(%ebp), %eax
	movl	%esp, (%eax)
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */

	/* Set user DS and ES */
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* IRET to user mode */
#ifdef __amd64__
	movq	%rdi, %rsp
	POPAQ
	iretq
#elif defined(__i386__)
	movl	8(%ebp), %esp
	popal
	iret
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */

.global exit_user_mode_asm
exit_user_mode_asm:

	/* Restore kernel mode state */
#ifdef __amd64__
	movq	%rdi, %rsp
	popfq
	POPAQ
#elif defined(__i386__)
	movl	4(%esp), %esp
	popfl
	popal
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */
	movw	$__DS, %ax
	movw	%ax, %ss

	/* Epilogue */
#ifdef __amd64__
	popq	%rbp
#elif defined(__i386__)
	popl	%ebp
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */
	ret

