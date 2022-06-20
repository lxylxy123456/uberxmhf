#include <xmhf.h>
#include <lhv.h>

.global lhv_guest_entry

lhv_guest_entry:
1:	vmcall
	jmp 1b

#ifdef __I386__
	push	%edi
#elif !defined(__AMD64__)
    #error "Unsupported Arch"
#endif /* !defined(__AMD64__) */
	call	lhv_guest_main
1:	jmp		1b		/* Should never return */

