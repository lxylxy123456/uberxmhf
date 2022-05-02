/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
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

/*
 * EMHF exception handler component low-level stubs
 */

#include <xmhf.h>
#include <xmhf-xcphandler.h>

.altmacro
.macro XtRtmEmitIdtStub vector skip_err_code
	.section .text

	//notes:
	//1. we only handle exceptions (0-31) and nothing else
	//2. descriptor type for all exception handlers are 0x8E
	//   i.e., they are all interrupt gates, so when we get called
	//   EFLAGS.IF = 0 (interrupts disabled)
	//3. there are no stack switches as everything in the hypervisor
	//	 is CPL=0
	//4. stack layout is as follows when we enter
	//	 EFLAGS	(32-bits)
	//   CS		(32-bits)
	//	 EIP	(32-bits)
	//	 error-code (32-bits, depends on the exception)

	XtRtmIdtStub&vector&:

		//push GPR values on stack to construct a
		//struct regs *r
		pushal

		movw	$(__DS), %ax
		movw	%ax, %ds			//load DS
		movl 	%esp, %eax			//EAX = struct regs *

		pushl 	%eax				//struct regs *r
		pushl	$0x&vector&			//vector
		call	xmhf_xcphandler_hub	//call C-land hub
		addl  	$0x08, %esp			//remove params from stack

		//restore GPRs
		popal

		/* If the exception puts an error code on the stack, ignore it */
		.if &skip_err_code&
		leal    4(%esp), %esp
		.endif

		//return from exception
		iretl

.endm

XtRtmEmitIdtStub 0  0
XtRtmEmitIdtStub 1  0
XtRtmEmitIdtStub 2  0
XtRtmEmitIdtStub 3  0
XtRtmEmitIdtStub 4  0
XtRtmEmitIdtStub 5  0
XtRtmEmitIdtStub 6  0
XtRtmEmitIdtStub 7  0
XtRtmEmitIdtStub 8  0
XtRtmEmitIdtStub 9  0
XtRtmEmitIdtStub a  1
XtRtmEmitIdtStub b  1
XtRtmEmitIdtStub c  1
XtRtmEmitIdtStub d  1
XtRtmEmitIdtStub e  1
XtRtmEmitIdtStub f  0
XtRtmEmitIdtStub 10 0
XtRtmEmitIdtStub 11 1
XtRtmEmitIdtStub 12 0
XtRtmEmitIdtStub 13 0
XtRtmEmitIdtStub 14 0
XtRtmEmitIdtStub 15 0
XtRtmEmitIdtStub 16 0
XtRtmEmitIdtStub 17 0
XtRtmEmitIdtStub 18 0
XtRtmEmitIdtStub 19 0
XtRtmEmitIdtStub 1a 0
XtRtmEmitIdtStub 1b 0
XtRtmEmitIdtStub 1c 0
XtRtmEmitIdtStub 1d 0
XtRtmEmitIdtStub 1e 0
XtRtmEmitIdtStub 1f 0
XtRtmEmitIdtStub 20 0
XtRtmEmitIdtStub 21 0
XtRtmEmitIdtStub 22 0
XtRtmEmitIdtStub 23 0
XtRtmEmitIdtStub 24 0
XtRtmEmitIdtStub 25 0
XtRtmEmitIdtStub 26 0
XtRtmEmitIdtStub 27 0
XtRtmEmitIdtStub 28 0
XtRtmEmitIdtStub 29 0
XtRtmEmitIdtStub 2a 0
XtRtmEmitIdtStub 2b 0
XtRtmEmitIdtStub 2c 0
XtRtmEmitIdtStub 2d 0
XtRtmEmitIdtStub 2e 0
XtRtmEmitIdtStub 2f 0
XtRtmEmitIdtStub 30 0
XtRtmEmitIdtStub 31 0
XtRtmEmitIdtStub 32 0
XtRtmEmitIdtStub 33 0
XtRtmEmitIdtStub 34 0
XtRtmEmitIdtStub 35 0
XtRtmEmitIdtStub 36 0
XtRtmEmitIdtStub 37 0
XtRtmEmitIdtStub 38 0
XtRtmEmitIdtStub 39 0
XtRtmEmitIdtStub 3a 0
XtRtmEmitIdtStub 3b 0
XtRtmEmitIdtStub 3c 0
XtRtmEmitIdtStub 3d 0
XtRtmEmitIdtStub 3e 0
XtRtmEmitIdtStub 3f 0
XtRtmEmitIdtStub 40 0
XtRtmEmitIdtStub 41 0
XtRtmEmitIdtStub 42 0
XtRtmEmitIdtStub 43 0
XtRtmEmitIdtStub 44 0
XtRtmEmitIdtStub 45 0
XtRtmEmitIdtStub 46 0
XtRtmEmitIdtStub 47 0
XtRtmEmitIdtStub 48 0
XtRtmEmitIdtStub 49 0
XtRtmEmitIdtStub 4a 0
XtRtmEmitIdtStub 4b 0
XtRtmEmitIdtStub 4c 0
XtRtmEmitIdtStub 4d 0
XtRtmEmitIdtStub 4e 0
XtRtmEmitIdtStub 4f 0
XtRtmEmitIdtStub 50 0
XtRtmEmitIdtStub 51 0
XtRtmEmitIdtStub 52 0
XtRtmEmitIdtStub 53 0
XtRtmEmitIdtStub 54 0
XtRtmEmitIdtStub 55 0
XtRtmEmitIdtStub 56 0
XtRtmEmitIdtStub 57 0
XtRtmEmitIdtStub 58 0
XtRtmEmitIdtStub 59 0
XtRtmEmitIdtStub 5a 0
XtRtmEmitIdtStub 5b 0
XtRtmEmitIdtStub 5c 0
XtRtmEmitIdtStub 5d 0
XtRtmEmitIdtStub 5e 0
XtRtmEmitIdtStub 5f 0
XtRtmEmitIdtStub 60 0
XtRtmEmitIdtStub 61 0
XtRtmEmitIdtStub 62 0
XtRtmEmitIdtStub 63 0
XtRtmEmitIdtStub 64 0
XtRtmEmitIdtStub 65 0
XtRtmEmitIdtStub 66 0
XtRtmEmitIdtStub 67 0
XtRtmEmitIdtStub 68 0
XtRtmEmitIdtStub 69 0
XtRtmEmitIdtStub 6a 0
XtRtmEmitIdtStub 6b 0
XtRtmEmitIdtStub 6c 0
XtRtmEmitIdtStub 6d 0
XtRtmEmitIdtStub 6e 0
XtRtmEmitIdtStub 6f 0
XtRtmEmitIdtStub 70 0
XtRtmEmitIdtStub 71 0
XtRtmEmitIdtStub 72 0
XtRtmEmitIdtStub 73 0
XtRtmEmitIdtStub 74 0
XtRtmEmitIdtStub 75 0
XtRtmEmitIdtStub 76 0
XtRtmEmitIdtStub 77 0
XtRtmEmitIdtStub 78 0
XtRtmEmitIdtStub 79 0
XtRtmEmitIdtStub 7a 0
XtRtmEmitIdtStub 7b 0
XtRtmEmitIdtStub 7c 0
XtRtmEmitIdtStub 7d 0
XtRtmEmitIdtStub 7e 0
XtRtmEmitIdtStub 7f 0
XtRtmEmitIdtStub 80 0
XtRtmEmitIdtStub 81 0
XtRtmEmitIdtStub 82 0
XtRtmEmitIdtStub 83 0
XtRtmEmitIdtStub 84 0
XtRtmEmitIdtStub 85 0
XtRtmEmitIdtStub 86 0
XtRtmEmitIdtStub 87 0
XtRtmEmitIdtStub 88 0
XtRtmEmitIdtStub 89 0
XtRtmEmitIdtStub 8a 0
XtRtmEmitIdtStub 8b 0
XtRtmEmitIdtStub 8c 0
XtRtmEmitIdtStub 8d 0
XtRtmEmitIdtStub 8e 0
XtRtmEmitIdtStub 8f 0
XtRtmEmitIdtStub 90 0
XtRtmEmitIdtStub 91 0
XtRtmEmitIdtStub 92 0
XtRtmEmitIdtStub 93 0
XtRtmEmitIdtStub 94 0
XtRtmEmitIdtStub 95 0
XtRtmEmitIdtStub 96 0
XtRtmEmitIdtStub 97 0
XtRtmEmitIdtStub 98 0
XtRtmEmitIdtStub 99 0
XtRtmEmitIdtStub 9a 0
XtRtmEmitIdtStub 9b 0
XtRtmEmitIdtStub 9c 0
XtRtmEmitIdtStub 9d 0
XtRtmEmitIdtStub 9e 0
XtRtmEmitIdtStub 9f 0
XtRtmEmitIdtStub a0 0
XtRtmEmitIdtStub a1 0
XtRtmEmitIdtStub a2 0
XtRtmEmitIdtStub a3 0
XtRtmEmitIdtStub a4 0
XtRtmEmitIdtStub a5 0
XtRtmEmitIdtStub a6 0
XtRtmEmitIdtStub a7 0
XtRtmEmitIdtStub a8 0
XtRtmEmitIdtStub a9 0
XtRtmEmitIdtStub aa 0
XtRtmEmitIdtStub ab 0
XtRtmEmitIdtStub ac 0
XtRtmEmitIdtStub ad 0
XtRtmEmitIdtStub ae 0
XtRtmEmitIdtStub af 0
XtRtmEmitIdtStub b0 0
XtRtmEmitIdtStub b1 0
XtRtmEmitIdtStub b2 0
XtRtmEmitIdtStub b3 0
XtRtmEmitIdtStub b4 0
XtRtmEmitIdtStub b5 0
XtRtmEmitIdtStub b6 0
XtRtmEmitIdtStub b7 0
XtRtmEmitIdtStub b8 0
XtRtmEmitIdtStub b9 0
XtRtmEmitIdtStub ba 0
XtRtmEmitIdtStub bb 0
XtRtmEmitIdtStub bc 0
XtRtmEmitIdtStub bd 0
XtRtmEmitIdtStub be 0
XtRtmEmitIdtStub bf 0
XtRtmEmitIdtStub c0 0
XtRtmEmitIdtStub c1 0
XtRtmEmitIdtStub c2 0
XtRtmEmitIdtStub c3 0
XtRtmEmitIdtStub c4 0
XtRtmEmitIdtStub c5 0
XtRtmEmitIdtStub c6 0
XtRtmEmitIdtStub c7 0
XtRtmEmitIdtStub c8 0
XtRtmEmitIdtStub c9 0
XtRtmEmitIdtStub ca 0
XtRtmEmitIdtStub cb 0
XtRtmEmitIdtStub cc 0
XtRtmEmitIdtStub cd 0
XtRtmEmitIdtStub ce 0
XtRtmEmitIdtStub cf 0
XtRtmEmitIdtStub d0 0
XtRtmEmitIdtStub d1 0
XtRtmEmitIdtStub d2 0
XtRtmEmitIdtStub d3 0
XtRtmEmitIdtStub d4 0
XtRtmEmitIdtStub d5 0
XtRtmEmitIdtStub d6 0
XtRtmEmitIdtStub d7 0
XtRtmEmitIdtStub d8 0
XtRtmEmitIdtStub d9 0
XtRtmEmitIdtStub da 0
XtRtmEmitIdtStub db 0
XtRtmEmitIdtStub dc 0
XtRtmEmitIdtStub dd 0
XtRtmEmitIdtStub de 0
XtRtmEmitIdtStub df 0
XtRtmEmitIdtStub e0 0
XtRtmEmitIdtStub e1 0
XtRtmEmitIdtStub e2 0
XtRtmEmitIdtStub e3 0
XtRtmEmitIdtStub e4 0
XtRtmEmitIdtStub e5 0
XtRtmEmitIdtStub e6 0
XtRtmEmitIdtStub e7 0
XtRtmEmitIdtStub e8 0
XtRtmEmitIdtStub e9 0
XtRtmEmitIdtStub ea 0
XtRtmEmitIdtStub eb 0
XtRtmEmitIdtStub ec 0
XtRtmEmitIdtStub ed 0
XtRtmEmitIdtStub ee 0
XtRtmEmitIdtStub ef 0
XtRtmEmitIdtStub f0 0
XtRtmEmitIdtStub f1 0
XtRtmEmitIdtStub f2 0
XtRtmEmitIdtStub f3 0
XtRtmEmitIdtStub f4 0
XtRtmEmitIdtStub f5 0
XtRtmEmitIdtStub f6 0
XtRtmEmitIdtStub f7 0
XtRtmEmitIdtStub f8 0
XtRtmEmitIdtStub f9 0
XtRtmEmitIdtStub fa 0
XtRtmEmitIdtStub fb 0
XtRtmEmitIdtStub fc 0
XtRtmEmitIdtStub fd 0
XtRtmEmitIdtStub fe 0
XtRtmEmitIdtStub ff 0


.section .data
	//EMHF interrupt descriptor table
	.global xmhf_xcphandler_idt
	xmhf_xcphandler_idt:
	.word xmhf_xcphandler_idt_end - xmhf_xcphandler_idt_start - 1
	.long xmhf_xcphandler_idt_start
	.align 16
	.global xmhf_xcphandler_idt_start
	xmhf_xcphandler_idt_start:
	.fill	1024, 1, 0
	xmhf_xcphandler_idt_end:


	.align 16
	.global xmhf_xcphandler_exceptionstubs
	xmhf_xcphandler_exceptionstubs:
	.long XtRtmIdtStub0
	.long XtRtmIdtStub1
	.long XtRtmIdtStub2
	.long XtRtmIdtStub3
	.long XtRtmIdtStub4
	.long XtRtmIdtStub5
	.long XtRtmIdtStub6
	.long XtRtmIdtStub7
	.long XtRtmIdtStub8
	.long XtRtmIdtStub9
	.long XtRtmIdtStuba
	.long XtRtmIdtStubb
	.long XtRtmIdtStubc
	.long XtRtmIdtStubd
	.long XtRtmIdtStube
	.long XtRtmIdtStubf
	.long XtRtmIdtStub10
	.long XtRtmIdtStub11
	.long XtRtmIdtStub12
	.long XtRtmIdtStub13
	.long XtRtmIdtStub14
	.long XtRtmIdtStub15
	.long XtRtmIdtStub16
	.long XtRtmIdtStub17
	.long XtRtmIdtStub18
	.long XtRtmIdtStub19
	.long XtRtmIdtStub1a
	.long XtRtmIdtStub1b
	.long XtRtmIdtStub1c
	.long XtRtmIdtStub1d
	.long XtRtmIdtStub1e
	.long XtRtmIdtStub1f
	.long XtRtmIdtStub20
	.long XtRtmIdtStub21
	.long XtRtmIdtStub22
	.long XtRtmIdtStub23
	.long XtRtmIdtStub24
	.long XtRtmIdtStub25
	.long XtRtmIdtStub26
	.long XtRtmIdtStub27
	.long XtRtmIdtStub28
	.long XtRtmIdtStub29
	.long XtRtmIdtStub2a
	.long XtRtmIdtStub2b
	.long XtRtmIdtStub2c
	.long XtRtmIdtStub2d
	.long XtRtmIdtStub2e
	.long XtRtmIdtStub2f
	.long XtRtmIdtStub30
	.long XtRtmIdtStub31
	.long XtRtmIdtStub32
	.long XtRtmIdtStub33
	.long XtRtmIdtStub34
	.long XtRtmIdtStub35
	.long XtRtmIdtStub36
	.long XtRtmIdtStub37
	.long XtRtmIdtStub38
	.long XtRtmIdtStub39
	.long XtRtmIdtStub3a
	.long XtRtmIdtStub3b
	.long XtRtmIdtStub3c
	.long XtRtmIdtStub3d
	.long XtRtmIdtStub3e
	.long XtRtmIdtStub3f
	.long XtRtmIdtStub40
	.long XtRtmIdtStub41
	.long XtRtmIdtStub42
	.long XtRtmIdtStub43
	.long XtRtmIdtStub44
	.long XtRtmIdtStub45
	.long XtRtmIdtStub46
	.long XtRtmIdtStub47
	.long XtRtmIdtStub48
	.long XtRtmIdtStub49
	.long XtRtmIdtStub4a
	.long XtRtmIdtStub4b
	.long XtRtmIdtStub4c
	.long XtRtmIdtStub4d
	.long XtRtmIdtStub4e
	.long XtRtmIdtStub4f
	.long XtRtmIdtStub50
	.long XtRtmIdtStub51
	.long XtRtmIdtStub52
	.long XtRtmIdtStub53
	.long XtRtmIdtStub54
	.long XtRtmIdtStub55
	.long XtRtmIdtStub56
	.long XtRtmIdtStub57
	.long XtRtmIdtStub58
	.long XtRtmIdtStub59
	.long XtRtmIdtStub5a
	.long XtRtmIdtStub5b
	.long XtRtmIdtStub5c
	.long XtRtmIdtStub5d
	.long XtRtmIdtStub5e
	.long XtRtmIdtStub5f
	.long XtRtmIdtStub60
	.long XtRtmIdtStub61
	.long XtRtmIdtStub62
	.long XtRtmIdtStub63
	.long XtRtmIdtStub64
	.long XtRtmIdtStub65
	.long XtRtmIdtStub66
	.long XtRtmIdtStub67
	.long XtRtmIdtStub68
	.long XtRtmIdtStub69
	.long XtRtmIdtStub6a
	.long XtRtmIdtStub6b
	.long XtRtmIdtStub6c
	.long XtRtmIdtStub6d
	.long XtRtmIdtStub6e
	.long XtRtmIdtStub6f
	.long XtRtmIdtStub70
	.long XtRtmIdtStub71
	.long XtRtmIdtStub72
	.long XtRtmIdtStub73
	.long XtRtmIdtStub74
	.long XtRtmIdtStub75
	.long XtRtmIdtStub76
	.long XtRtmIdtStub77
	.long XtRtmIdtStub78
	.long XtRtmIdtStub79
	.long XtRtmIdtStub7a
	.long XtRtmIdtStub7b
	.long XtRtmIdtStub7c
	.long XtRtmIdtStub7d
	.long XtRtmIdtStub7e
	.long XtRtmIdtStub7f
	.long XtRtmIdtStub80
	.long XtRtmIdtStub81
	.long XtRtmIdtStub82
	.long XtRtmIdtStub83
	.long XtRtmIdtStub84
	.long XtRtmIdtStub85
	.long XtRtmIdtStub86
	.long XtRtmIdtStub87
	.long XtRtmIdtStub88
	.long XtRtmIdtStub89
	.long XtRtmIdtStub8a
	.long XtRtmIdtStub8b
	.long XtRtmIdtStub8c
	.long XtRtmIdtStub8d
	.long XtRtmIdtStub8e
	.long XtRtmIdtStub8f
	.long XtRtmIdtStub90
	.long XtRtmIdtStub91
	.long XtRtmIdtStub92
	.long XtRtmIdtStub93
	.long XtRtmIdtStub94
	.long XtRtmIdtStub95
	.long XtRtmIdtStub96
	.long XtRtmIdtStub97
	.long XtRtmIdtStub98
	.long XtRtmIdtStub99
	.long XtRtmIdtStub9a
	.long XtRtmIdtStub9b
	.long XtRtmIdtStub9c
	.long XtRtmIdtStub9d
	.long XtRtmIdtStub9e
	.long XtRtmIdtStub9f
	.long XtRtmIdtStuba0
	.long XtRtmIdtStuba1
	.long XtRtmIdtStuba2
	.long XtRtmIdtStuba3
	.long XtRtmIdtStuba4
	.long XtRtmIdtStuba5
	.long XtRtmIdtStuba6
	.long XtRtmIdtStuba7
	.long XtRtmIdtStuba8
	.long XtRtmIdtStuba9
	.long XtRtmIdtStubaa
	.long XtRtmIdtStubab
	.long XtRtmIdtStubac
	.long XtRtmIdtStubad
	.long XtRtmIdtStubae
	.long XtRtmIdtStubaf
	.long XtRtmIdtStubb0
	.long XtRtmIdtStubb1
	.long XtRtmIdtStubb2
	.long XtRtmIdtStubb3
	.long XtRtmIdtStubb4
	.long XtRtmIdtStubb5
	.long XtRtmIdtStubb6
	.long XtRtmIdtStubb7
	.long XtRtmIdtStubb8
	.long XtRtmIdtStubb9
	.long XtRtmIdtStubba
	.long XtRtmIdtStubbb
	.long XtRtmIdtStubbc
	.long XtRtmIdtStubbd
	.long XtRtmIdtStubbe
	.long XtRtmIdtStubbf
	.long XtRtmIdtStubc0
	.long XtRtmIdtStubc1
	.long XtRtmIdtStubc2
	.long XtRtmIdtStubc3
	.long XtRtmIdtStubc4
	.long XtRtmIdtStubc5
	.long XtRtmIdtStubc6
	.long XtRtmIdtStubc7
	.long XtRtmIdtStubc8
	.long XtRtmIdtStubc9
	.long XtRtmIdtStubca
	.long XtRtmIdtStubcb
	.long XtRtmIdtStubcc
	.long XtRtmIdtStubcd
	.long XtRtmIdtStubce
	.long XtRtmIdtStubcf
	.long XtRtmIdtStubd0
	.long XtRtmIdtStubd1
	.long XtRtmIdtStubd2
	.long XtRtmIdtStubd3
	.long XtRtmIdtStubd4
	.long XtRtmIdtStubd5
	.long XtRtmIdtStubd6
	.long XtRtmIdtStubd7
	.long XtRtmIdtStubd8
	.long XtRtmIdtStubd9
	.long XtRtmIdtStubda
	.long XtRtmIdtStubdb
	.long XtRtmIdtStubdc
	.long XtRtmIdtStubdd
	.long XtRtmIdtStubde
	.long XtRtmIdtStubdf
	.long XtRtmIdtStube0
	.long XtRtmIdtStube1
	.long XtRtmIdtStube2
	.long XtRtmIdtStube3
	.long XtRtmIdtStube4
	.long XtRtmIdtStube5
	.long XtRtmIdtStube6
	.long XtRtmIdtStube7
	.long XtRtmIdtStube8
	.long XtRtmIdtStube9
	.long XtRtmIdtStubea
	.long XtRtmIdtStubeb
	.long XtRtmIdtStubec
	.long XtRtmIdtStubed
	.long XtRtmIdtStubee
	.long XtRtmIdtStubef
	.long XtRtmIdtStubf0
	.long XtRtmIdtStubf1
	.long XtRtmIdtStubf2
	.long XtRtmIdtStubf3
	.long XtRtmIdtStubf4
	.long XtRtmIdtStubf5
	.long XtRtmIdtStubf6
	.long XtRtmIdtStubf7
	.long XtRtmIdtStubf8
	.long XtRtmIdtStubf9
	.long XtRtmIdtStubfa
	.long XtRtmIdtStubfb
	.long XtRtmIdtStubfc
	.long XtRtmIdtStubfd
	.long XtRtmIdtStubfe
	.long XtRtmIdtStubff
