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
    //   is CPL=0
    //4. stack layout is as follows when we enter
    //   EFLAGS (32-bits)
    //   CS     (32-bits)
    //   EIP    (32-bits)
    //   error-code (32-bits, depends on the exception)

    XtRtmIdtStub&vector&:

        // push GPR values on stack to construct struct regs *r
        PUSHAQ

        movq    %rsp, %rsi          // RSI (second argument) = struct regs *
        movq    $0x&vector&, %rdi   // RDI (first argument) = vector

        xorq    %rax, %rax
        movw    %ds, %ax
        pushq   %rax                //save DS
        movw    %es, %ax
        pushq   %rax                //save ES
        movw    %fs, %ax
        pushq   %rax                //save FS
        movw    %gs, %ax
        pushq   %rax                //save GS

        movw    $(__DS), %ax
        movw    %ax, %ds            //load DS
        movw    %ax, %es            //load ES
        movw    %ax, %fs            //load FS
        movw    %ax, %gs            //load GS

        call    xmhf_xcphandler_hub // call C-land hub

        movw    %ds, %ax
        popq    %rax
        movw    %ax, %gs            //restore GS
        popq    %rax
        movw    %ax, %fs            //restore FS
        popq    %rax
        movw    %ax, %es            //restore ES
        popq    %rax
        movw    %ax, %ds            //restore DS

        // pop GPR values, except RSP
        POPAQ

        /* If the exception puts an error code on the stack, ignore it */
        .if &skip_err_code&
        leaq    8(%rsp), %rsp
        .endif

        // return from exception
        iretq

.endm

XtRtmEmitIdtStub 0  0
XtRtmEmitIdtStub 1  0
XtRtmEmitIdtStub 2  0
XtRtmEmitIdtStub 3  0
XtRtmEmitIdtStub 4  0
XtRtmEmitIdtStub 5  0
XtRtmEmitIdtStub 6  0
XtRtmEmitIdtStub 7  0
XtRtmEmitIdtStub 8  1
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
    .quad xmhf_xcphandler_idt_start
    .align 16
    .global xmhf_xcphandler_idt_start
    xmhf_xcphandler_idt_start:
    .fill   (EMHF_XCPHANDLER_MAXEXCEPTIONS * 16), 1, 0
    xmhf_xcphandler_idt_end:


    .align 16
    .global xmhf_xcphandler_exceptionstubs
    xmhf_xcphandler_exceptionstubs:
    .quad XtRtmIdtStub0
    .quad XtRtmIdtStub1
    .quad XtRtmIdtStub2
    .quad XtRtmIdtStub3
    .quad XtRtmIdtStub4
    .quad XtRtmIdtStub5
    .quad XtRtmIdtStub6
    .quad XtRtmIdtStub7
    .quad XtRtmIdtStub8
    .quad XtRtmIdtStub9
    .quad XtRtmIdtStuba
    .quad XtRtmIdtStubb
    .quad XtRtmIdtStubc
    .quad XtRtmIdtStubd
    .quad XtRtmIdtStube
    .quad XtRtmIdtStubf
    .quad XtRtmIdtStub10
    .quad XtRtmIdtStub11
    .quad XtRtmIdtStub12
    .quad XtRtmIdtStub13
    .quad XtRtmIdtStub14
    .quad XtRtmIdtStub15
    .quad XtRtmIdtStub16
    .quad XtRtmIdtStub17
    .quad XtRtmIdtStub18
    .quad XtRtmIdtStub19
    .quad XtRtmIdtStub1a
    .quad XtRtmIdtStub1b
    .quad XtRtmIdtStub1c
    .quad XtRtmIdtStub1d
    .quad XtRtmIdtStub1e
    .quad XtRtmIdtStub1f
	.quad XtRtmIdtStub20
	.quad XtRtmIdtStub21
	.quad XtRtmIdtStub22
	.quad XtRtmIdtStub23
	.quad XtRtmIdtStub24
	.quad XtRtmIdtStub25
	.quad XtRtmIdtStub26
	.quad XtRtmIdtStub27
	.quad XtRtmIdtStub28
	.quad XtRtmIdtStub29
	.quad XtRtmIdtStub2a
	.quad XtRtmIdtStub2b
	.quad XtRtmIdtStub2c
	.quad XtRtmIdtStub2d
	.quad XtRtmIdtStub2e
	.quad XtRtmIdtStub2f
	.quad XtRtmIdtStub30
	.quad XtRtmIdtStub31
	.quad XtRtmIdtStub32
	.quad XtRtmIdtStub33
	.quad XtRtmIdtStub34
	.quad XtRtmIdtStub35
	.quad XtRtmIdtStub36
	.quad XtRtmIdtStub37
	.quad XtRtmIdtStub38
	.quad XtRtmIdtStub39
	.quad XtRtmIdtStub3a
	.quad XtRtmIdtStub3b
	.quad XtRtmIdtStub3c
	.quad XtRtmIdtStub3d
	.quad XtRtmIdtStub3e
	.quad XtRtmIdtStub3f
	.quad XtRtmIdtStub40
	.quad XtRtmIdtStub41
	.quad XtRtmIdtStub42
	.quad XtRtmIdtStub43
	.quad XtRtmIdtStub44
	.quad XtRtmIdtStub45
	.quad XtRtmIdtStub46
	.quad XtRtmIdtStub47
	.quad XtRtmIdtStub48
	.quad XtRtmIdtStub49
	.quad XtRtmIdtStub4a
	.quad XtRtmIdtStub4b
	.quad XtRtmIdtStub4c
	.quad XtRtmIdtStub4d
	.quad XtRtmIdtStub4e
	.quad XtRtmIdtStub4f
	.quad XtRtmIdtStub50
	.quad XtRtmIdtStub51
	.quad XtRtmIdtStub52
	.quad XtRtmIdtStub53
	.quad XtRtmIdtStub54
	.quad XtRtmIdtStub55
	.quad XtRtmIdtStub56
	.quad XtRtmIdtStub57
	.quad XtRtmIdtStub58
	.quad XtRtmIdtStub59
	.quad XtRtmIdtStub5a
	.quad XtRtmIdtStub5b
	.quad XtRtmIdtStub5c
	.quad XtRtmIdtStub5d
	.quad XtRtmIdtStub5e
	.quad XtRtmIdtStub5f
	.quad XtRtmIdtStub60
	.quad XtRtmIdtStub61
	.quad XtRtmIdtStub62
	.quad XtRtmIdtStub63
	.quad XtRtmIdtStub64
	.quad XtRtmIdtStub65
	.quad XtRtmIdtStub66
	.quad XtRtmIdtStub67
	.quad XtRtmIdtStub68
	.quad XtRtmIdtStub69
	.quad XtRtmIdtStub6a
	.quad XtRtmIdtStub6b
	.quad XtRtmIdtStub6c
	.quad XtRtmIdtStub6d
	.quad XtRtmIdtStub6e
	.quad XtRtmIdtStub6f
	.quad XtRtmIdtStub70
	.quad XtRtmIdtStub71
	.quad XtRtmIdtStub72
	.quad XtRtmIdtStub73
	.quad XtRtmIdtStub74
	.quad XtRtmIdtStub75
	.quad XtRtmIdtStub76
	.quad XtRtmIdtStub77
	.quad XtRtmIdtStub78
	.quad XtRtmIdtStub79
	.quad XtRtmIdtStub7a
	.quad XtRtmIdtStub7b
	.quad XtRtmIdtStub7c
	.quad XtRtmIdtStub7d
	.quad XtRtmIdtStub7e
	.quad XtRtmIdtStub7f
	.quad XtRtmIdtStub80
	.quad XtRtmIdtStub81
	.quad XtRtmIdtStub82
	.quad XtRtmIdtStub83
	.quad XtRtmIdtStub84
	.quad XtRtmIdtStub85
	.quad XtRtmIdtStub86
	.quad XtRtmIdtStub87
	.quad XtRtmIdtStub88
	.quad XtRtmIdtStub89
	.quad XtRtmIdtStub8a
	.quad XtRtmIdtStub8b
	.quad XtRtmIdtStub8c
	.quad XtRtmIdtStub8d
	.quad XtRtmIdtStub8e
	.quad XtRtmIdtStub8f
	.quad XtRtmIdtStub90
	.quad XtRtmIdtStub91
	.quad XtRtmIdtStub92
	.quad XtRtmIdtStub93
	.quad XtRtmIdtStub94
	.quad XtRtmIdtStub95
	.quad XtRtmIdtStub96
	.quad XtRtmIdtStub97
	.quad XtRtmIdtStub98
	.quad XtRtmIdtStub99
	.quad XtRtmIdtStub9a
	.quad XtRtmIdtStub9b
	.quad XtRtmIdtStub9c
	.quad XtRtmIdtStub9d
	.quad XtRtmIdtStub9e
	.quad XtRtmIdtStub9f
	.quad XtRtmIdtStuba0
	.quad XtRtmIdtStuba1
	.quad XtRtmIdtStuba2
	.quad XtRtmIdtStuba3
	.quad XtRtmIdtStuba4
	.quad XtRtmIdtStuba5
	.quad XtRtmIdtStuba6
	.quad XtRtmIdtStuba7
	.quad XtRtmIdtStuba8
	.quad XtRtmIdtStuba9
	.quad XtRtmIdtStubaa
	.quad XtRtmIdtStubab
	.quad XtRtmIdtStubac
	.quad XtRtmIdtStubad
	.quad XtRtmIdtStubae
	.quad XtRtmIdtStubaf
	.quad XtRtmIdtStubb0
	.quad XtRtmIdtStubb1
	.quad XtRtmIdtStubb2
	.quad XtRtmIdtStubb3
	.quad XtRtmIdtStubb4
	.quad XtRtmIdtStubb5
	.quad XtRtmIdtStubb6
	.quad XtRtmIdtStubb7
	.quad XtRtmIdtStubb8
	.quad XtRtmIdtStubb9
	.quad XtRtmIdtStubba
	.quad XtRtmIdtStubbb
	.quad XtRtmIdtStubbc
	.quad XtRtmIdtStubbd
	.quad XtRtmIdtStubbe
	.quad XtRtmIdtStubbf
	.quad XtRtmIdtStubc0
	.quad XtRtmIdtStubc1
	.quad XtRtmIdtStubc2
	.quad XtRtmIdtStubc3
	.quad XtRtmIdtStubc4
	.quad XtRtmIdtStubc5
	.quad XtRtmIdtStubc6
	.quad XtRtmIdtStubc7
	.quad XtRtmIdtStubc8
	.quad XtRtmIdtStubc9
	.quad XtRtmIdtStubca
	.quad XtRtmIdtStubcb
	.quad XtRtmIdtStubcc
	.quad XtRtmIdtStubcd
	.quad XtRtmIdtStubce
	.quad XtRtmIdtStubcf
	.quad XtRtmIdtStubd0
	.quad XtRtmIdtStubd1
	.quad XtRtmIdtStubd2
	.quad XtRtmIdtStubd3
	.quad XtRtmIdtStubd4
	.quad XtRtmIdtStubd5
	.quad XtRtmIdtStubd6
	.quad XtRtmIdtStubd7
	.quad XtRtmIdtStubd8
	.quad XtRtmIdtStubd9
	.quad XtRtmIdtStubda
	.quad XtRtmIdtStubdb
	.quad XtRtmIdtStubdc
	.quad XtRtmIdtStubdd
	.quad XtRtmIdtStubde
	.quad XtRtmIdtStubdf
	.quad XtRtmIdtStube0
	.quad XtRtmIdtStube1
	.quad XtRtmIdtStube2
	.quad XtRtmIdtStube3
	.quad XtRtmIdtStube4
	.quad XtRtmIdtStube5
	.quad XtRtmIdtStube6
	.quad XtRtmIdtStube7
	.quad XtRtmIdtStube8
	.quad XtRtmIdtStube9
	.quad XtRtmIdtStubea
	.quad XtRtmIdtStubeb
	.quad XtRtmIdtStubec
	.quad XtRtmIdtStubed
	.quad XtRtmIdtStubee
	.quad XtRtmIdtStubef
	.quad XtRtmIdtStubf0
	.quad XtRtmIdtStubf1
	.quad XtRtmIdtStubf2
	.quad XtRtmIdtStubf3
	.quad XtRtmIdtStubf4
	.quad XtRtmIdtStubf5
	.quad XtRtmIdtStubf6
	.quad XtRtmIdtStubf7
	.quad XtRtmIdtStubf8
	.quad XtRtmIdtStubf9
	.quad XtRtmIdtStubfa
	.quad XtRtmIdtStubfb
	.quad XtRtmIdtStubfc
	.quad XtRtmIdtStubfd
	.quad XtRtmIdtStubfe
	.quad XtRtmIdtStubff
