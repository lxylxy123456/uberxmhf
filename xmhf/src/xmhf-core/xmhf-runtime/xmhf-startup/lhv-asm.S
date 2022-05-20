#include <xmhf.h>
#include <lhv.h>

.global vmexit_asm
.global vmlaunch_asm
.global vmresume_asm

vmexit_asm:
1:	jmp 1b

.altmacro
.macro vmentry_asm_template inst_name inst_num

inst_name&_asm:
#ifdef __XMHF_AMD64__
	movq	%rdi, %rsp
	popq	%rdi
	popq	%rsi
	popq	%rbp
	leaq	8(%rsp), %rsp
	popq	%rbx
	popq	%rdx
	popq	%rcx
	popq	%rax
	popq	%r8
	popq	%r9
	popq	%r10
	popq	%r11
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
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

