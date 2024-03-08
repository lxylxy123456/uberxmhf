/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * Copyright (c) 2023 Eric Li
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

// init low-level support routines for UEFI
// author: Eric Li (xiaoyili@andrew.cmu.edu)

#include <xmhf.h>

.extern midtable_numentries
.extern mp_cstartup
.extern midtable

//---AP boot-strap code---------------------------------------------------------
.section .text
.code16
.global _ap_bootstrap_start
_ap_bootstrap_start:
	jmp		ap_bootstrap_bypassdata
_ap_gdtdesc:
	.word	_ap_gdt_end - _ap_gdt_start - 1
	.long	_ap_gdt_start - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4)
    .align 16
_ap_gdt_start:
	.quad	0x0000000000000000  /* 0x00: NULL selector */
	.quad	0x00af9a000000ffff  /* 0x08: 64-bit CODE selector */
	.quad	0x00cf9a000000ffff  /* 0x10: 32-bit CODE selector */
	.quad	0x00cf92000000ffff  /* 0x18: 32-bit DATA selector */
	.quad	0x0000000000000000  /* 0x20: TSS low (unused) */
	.quad	0x0000000000000000  /* 0x28: TSS high (unused) */
_ap_gdt_end:
	.word	0

ap_bootstrap_bypassdata:

	/*
	 * Starting APs in bootloader in UEFI is not supported. The following code
	 * until init_core_lowlevel_setup() is not tested.
	 */
	ud2

	/* Set segment registers for real mode */
	movw	$AP_BOOTSTRAP_CODE_SEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	$0xFFFF, %sp
	movw	$0x4000, %ax
	movw	%ax, %ss

	/* Load GDT */
	movw	$(_ap_gdtdesc - _ap_bootstrap_start), %si
	lgdt	(%si)

	/* Enable protected mode */
	movl	%cr0, %eax
	orl		$0x1, %eax
	movl	%eax, %cr0

	/* Jump to 32-bit code */
	jmpl	$0x10, $(_ap_clear_pipe - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4))

.code32
_ap_clear_pipe:
	/* Set segment registers for protected mode */
	movw	$0x18, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	//TODO: movl $init_core_lowlevel_setup, %eax
	jmpl	*%eax
0:
    //we should never get here, if so just halt
    hlt
    jmp		0b

.global _ap_bootstrap_end
_ap_bootstrap_end:
	nop
	nop
	nop
	nop

//---init_core_lowlevel_setup---------------------------------------------------
.section .text
.code64
.global init_core_lowlevel_setup
init_core_lowlevel_setup:

	//load our gdt
	leaq	init_gdt(%rip), %rax
	lgdt	(%rax)

	//load segment registers (probably unnecessary)
	movw	$0x18, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	//get hold of local APIC id
	movl	$(MSR_APIC_BASE), %ecx
	rdmsr
	andl	$0xFFFFF000, %eax
	orl		$0x20, %eax
	movl	(%eax), %eax
	shrl	$24, %eax

	//get vcpu virtual address of this CPU/core
	movl	midtable_numentries(%rip), %edx
	leaq	midtable(%rip), %rbx
	xorq	%rcx, %rcx

getvcpuloop:
#ifdef __XMHF_I386__
	movl	0x0(%rbx, %rcx, 8), %ebp	//ebp contains the lapic id
#elif defined(__XMHF_AMD64__)
	leaq	0x0(%rbx, %rcx, 8), %rsp
	movl	0x0(%rsp, %rcx, 8), %ebp	//ebp contains the lapic id
#else
    #error "Unsupported Arch"
#endif /* __I386__ */
	cmpl	%eax, %ebp
	jz		gotvcpu
	incq	%rcx
	cmpq	%rdx, %rcx
    jb		getvcpuloop
0:
    //we should never get here, if so just halt
    hlt
    jmp		0b

gotvcpu:
#ifdef __XMHF_I386__
	movq	0x4(%rbx, %rcx, 8), %rdi	//rdi contains vcpu pointer
#elif defined(__XMHF_AMD64__)
	movq	0x8(%rsp, %rcx, 8), %rdi	//rdi contains vcpu pointer
#else
    #error "Unsupported Arch"
#endif /* __I386__ */
	movq	(%rdi), %rsp	//load stack for this CPU
	call	mp_cstartup
0:
	//we should never get here, if so just halt
	hlt
    jmp		0b



//------------------------------------------------------------------------------
.section  .data

.global init_gdt
.global init_gdt_base
.global init_gdt_start

    //the GDT
  init_gdt:
    .word   init_gdt_end - init_gdt_start - 1
  init_gdt_base:
    // Should be init_gdt_start, but 0 here and will be set up by C code later,
    // because UEFI applications/drivers need to be relocatable.
    .quad 0

    .align  16
  init_gdt_start:
    .quad   0x0000000000000000  /* 0x00: NULL selector */
    .quad   0x00af9a000000ffff  /* 0x08: 64-bit CODE selector */
    .quad   0x00cf9a000000ffff  /* 0x10: 32-bit CODE selector */
    .quad   0x00cf92000000ffff  /* 0x18: 32-bit DATA selector */
    .quad   0x0000000000000000  /* 0x20: TSS low (unused) */
    .quad   0x0000000000000000  /* 0x28: TSS high (unused) */
  init_gdt_end:
