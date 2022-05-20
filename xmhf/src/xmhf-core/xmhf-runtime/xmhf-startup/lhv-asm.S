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
1:	jmp 1b	// TODO
#elif defined(__I386__)
	movl	4(%esp), %esp
	popal
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	inst_name
#ifdef __XMHF_AMD64__
1:	jmp 1b	// TODO
#elif defined(__I386__)
	je		2f			/* ZF set means VMfailValid */
	jb		3f			/* CF set means VMfailInvalid */
1:	jmp		1b			/* Unknown error */
2:	pushl	$1
	jmp		4f
3:	pushl	$0
	jmp		4f
4:	pushl	$&inst_num&	/* vmlaunch = 0, vmresume = 1 */
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* __XMHF_AMD64__ */
	call	vmentry_error
1:	jmp		1b			/* Should never return */

.endm

vmentry_asm_template vmlaunch 0
vmentry_asm_template vmresume 1

