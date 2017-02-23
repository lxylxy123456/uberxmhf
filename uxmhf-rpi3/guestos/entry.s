/*
	entry stub

	author: amit vasudevan (amitvasudevan@acm.org)
*/

.globl entry
entry:

	/* load stack and start C land */
	ldr sp, =stack_top
	bl main

halt:
	b halt




.globl mmio_write32
mmio_write32:
    str r1,[r0]
    bx lr

.globl mmio_read32
mmio_read32:
    ldr r0,[r0]
    bx lr




.global hypcall
hypcall:
	hvc #0
	bx lr

.global svccall
svccall:
	svc #0
	bx lr

.global cpumodeswitch_svc2usr
cpumodeswitch_svc2usr:
	cps	#0x10
	ldr sp, =usrstack_top
	blx	r0


.globl sysreg_read_cpsr
sysreg_read_cpsr:
	mrs r0, cpsr
	bx lr

.global sysreg_read_vbar
sysreg_read_vbar:
	mrc p15, 0, r0, c12, c0, 0
	bx lr

.global sysreg_write_vbar
sysreg_write_vbar:
	mcr p15, 0, r0, c12, c0, 0
	bx lr


.section ".stack"

	.balign 8
	.global stack
	stack:	.space	256
	.global stack_top
	stack_top:

	.balign 8
	.global usrstack
	usrstack:	.space	256
	.global usrstack_top
	usrstack_top:
