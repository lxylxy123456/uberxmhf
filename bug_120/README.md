# Windows 11 DRT blue screen

## Scope
* Windows 11 UEFI
* Dell 7050
* `xmhf64 64db0f61f`
* `xmhf64-dev 9d905dc3a`
* DRT
* Does not matter: DMAP

## Behavior

This bug is first found in `bug_071`. See section `### DMAP`.

After Windows 11 boots for some time, see blue screen. Reason
`KMODE_EXCEPTION_NOT_HANDLED`. This bug only happens when DRT enabled.

## Debugging

By modifying exception bitmap, can intercept all exceptions in guest mode.

Git `xmhf64-dev 6446f3e7e`, serial `20230508212546`. Looks like the blue screen
is caused by `#DE`. First there are 125 `#GP(0)` exceptions (0x80000b0d), with
period of 25. Then there is a `#DE` exception (0x80000300).

Git `xmhf64-dev d865531fe`, serial `20230509014612`, print the page of code
that cause `#DE`.

Using `objdump -b binary -m i386:x86-64 -D FILE`, can see the assembly are like
```
fffff8038255d84c:	48 83 ec 28          	sub    $0x28,%rsp
fffff8038255d850:	0f ae e8             	lfence 
fffff8038255d853:	83 3d ea 68 10 00 00 	cmpl   $0x0,0x1068ea(%rip)        # 0xfffff80382664144
fffff8038255d85a:	0f 85 6c ce 01 00    	jne    0xfffff8038257a6cc
fffff8038255d860:	0f b6 15 39 d2 0f 00 	movzbl 0xfd239(%rip),%edx        # 0xfffff8038265aaa0
fffff8038255d867:	0f b6 05 26 c1 0a 00 	movzbl 0xac126(%rip),%eax        # 0xfffff80382609994
fffff8038255d86e:	0b d0                	or     %eax,%edx
fffff8038255d870:	8b ca                	mov    %edx,%ecx
fffff8038255d872:	f7 d9                	neg    %ecx
fffff8038255d874:	45 1b c0             	sbb    %r8d,%r8d
fffff8038255d877:	41 83 e0 ee          	and    $0xffffffee,%r8d
fffff8038255d87b:	41 83 c0 11          	add    $0x11,%r8d
fffff8038255d87f:	d1 ca                	ror    %edx
fffff8038255d881:	8b c2                	mov    %edx,%eax
fffff8038255d883:	99                   	cltd   
fffff8038255d884:	41 f7 f8             	idiv   %r8d
fffff8038255d887:	89 44 24 30          	mov    %eax,0x30(%rsp)
fffff8038255d88b:	eb 00                	jmp    0xfffff8038255d88d
fffff8038255d88d:	48 83 c4 28          	add    $0x28,%rsp
fffff8038255d891:	c3                   	ret    
```

Unfortunately, looks like `#DE` also happens when DRT is off. (When DRT is off,
Windows can boot correctly). Thus, `#DE` is not causing this problem. I guess
Windows' exception is not hardware exception.

### VT-d

In `xmhf64-dev 175e7a28d`, always perform `vmx_eap_zap()` (even without DRT).
Compile with no DRT and Windows 11 still boots. Thus, this bug is likely not
related to DRT hiding VT-d.

I think we need to debug using Microsoft provided tools like WinDbg.

### WinDbg

Install WinDbg on Windows 11, then open the BlueScreen dump file. Can see in
stack (most recent call first, line below calls line above):
```
nt!KeBugCheckEx
nt!HvlpVtlCallExceptionHandler+0x22
nt!RtlpExecuteHandlerForException+0xf
nt!RtlDispatchException+0x2f3
nt!KiDispatchException_0x1b1
nt!KxExceptionDispatchOnExceptionStack+0x12
nt!KiExceptionDispatchOnExceptionStackContinue
nt!KiExceptionDispatch+0x135
nt!KiInvalidOpcodeFault+0x321
intelppm!MWaitIdle+0x12
intelppm!AcpiCStateIdleExecute+0x24
nt!PpmIdleExecuteTransition+0x1132
nt!PoIdle+0x63d
nt!KiIdleLoop+0x54
```

From the function names, we can guess that MWAIT instruction causes an `#UD`
exception. Using WinDbg can also show disassembly of `intelppm!MWaitIdle`.
Can see that `#UD` exception happens in MONITOR instruction.
```
    intelppm!MWaitIdle:
fffff800`a5d01550 448bca       mov     r9d, edx
fffff800`a5d01553 4c8bc1       mov     r8, rcx
fffff800`a5d01556 85d2         test    edx, edx
fffff800`a5d01558 7532         jne     intelppm!MWaitIdle+0x3c (fffff800a5d0158c)
fffff800`a5d0155a 498b4008     mov     rax, qword ptr [r8+8]
fffff800`a5d0155e 33c9         xor     ecx, ecx
fffff800`a5d01560 33d2         xor     edx, edx
fffff800`a5d01562 0f01c8       monitor rax, rcx, rdx
fffff800`a5d01565 418b4804     mov     ecx, dword ptr [r8+4]
fffff800`a5d01569 418b00       mov     eax, dword ptr [r8]
fffff800`a5d0156c 0f01c9       mwait   rax, rcx
fffff800`a5d0156f 4585c9       test    r9d, r9d
fffff800`a5d01572 7505         jne     intelppm!MWaitIdle+0x29 (fffff800a5d01579)
fffff800`a5d01574 0faee8       lfence  
fffff800`a5d01577 c3           ret     
fffff800`a5d01578 cc           int     3
fffff800`a5d01579 498bd1       mov     rdx, r9
fffff800`a5d0157c 498bc1       mov     rax, r9
fffff800`a5d0157f 48c1ea20     shr     rdx, 20h
fffff800`a5d01583 b948000000   mov     ecx, 48h
fffff800`a5d01588 0f30         wrmsr   
fffff800`a5d0158a c3           ret     
fffff800`a5d0158b cc           int     3
fffff800`a5d0158c 33c0         xor     eax, eax
fffff800`a5d0158e b948000000   mov     ecx, 48h
fffff800`a5d01593 33d2         xor     edx, edx
fffff800`a5d01595 0f30         wrmsr   
fffff800`a5d01597 ebc1         jmp     intelppm!MWaitIdle+0xa (fffff800a5d0155a)
fffff800`a5d01599 cc           int     3
fffff800`a5d0159a cc           int     3
fffff800`a5d0159b cc           int     3
fffff800`a5d0159c cc           int     3
fffff800`a5d0159d cc           int     3
fffff800`a5d0159e cc           int     3
fffff800`a5d0159f cc           int     3
```

Intel TXT manual says MONITOR/MWAIT may be used to start RLPs, so we should
check CPUID and MSR bits related to this feature.
* MSR `IA32_MISC_ENABLE` (1A0H) bit 18: ENABLE MONITOR FSM (R/W)
* `CPUID.01H:ECX[bit 3]`: MONITOR/MWAIT
* MLE/SINIT Capabilities Field bit 1:
  Support for RLP wakeup using MONITOR address (SinitMleData.RlpWakeupAddr)

Intel TXT manual also says MSR `IA32_MISC_ENABLE` should be saved before TXT
launch and restored afterwards. However, XMHF does not do so (relevant code
from tboot are commented).

In `xmhf64-dev 652136da2`, print `IA32_MISC_ENABLE` for all CPUs. DRT off:
```
CPU(0x04): LXY: MSR_IA32_MISC_ENABLE = 0x4000840089
CPU(0x02): LXY: MSR_IA32_MISC_ENABLE = 0x4000840089
CPU(0x00): LXY: MSR_IA32_MISC_ENABLE = 0x4000840089
CPU(0x06): LXY: MSR_IA32_MISC_ENABLE = 0x4000840089
```

DRT on:
```
CPU(0x02): LXY: MSR_IA32_MISC_ENABLE = 0x4000840088
CPU(0x00): LXY: MSR_IA32_MISC_ENABLE = 0x4000800088
CPU(0x06): LXY: MSR_IA32_MISC_ENABLE = 0x4000840088
CPU(0x04): LXY: MSR_IA32_MISC_ENABLE = 0x4000840088
```

We can see that when DRT is on, CPU 0's MSR `IA32_MISC_ENABLE` bit 18 is
cleared. This is different from APs. This probably causes the trouble.

In `xmhf64 b8c3492d7`, add code to restore `IA32_MISC_ENABLE` after TXT launch.
However, now Windows 11 sees a different blue screen reason:
`CRITICAL_SERVICE_FAILED`.

#### Tboot bug

Looks like tboot incorrectly defines `saved_misc_enable_msr` as `uint32_t`.
Since it is an MSR, it should be `uint64_t`. In XMHF, this results in
`MSR_IA32_MISC_ENABLE = 0x000840089` being restored, but
`MSR_IA32_MISC_ENABLE = 0x4000840089` should be restored instead. This bug
appears in the latest tboot-1.11.1.

Fixed the bug in `xmhf64 a7fb6764b`.

Write bug report for tboot in `tboot_bug.txt`, sent to
`tboot-devel@lists.sourceforge.net`. I then receive an email saying the bug
report is held due to "Post by non-member to a members-only list".

### Windows 11 Future

Installed Windows 10 for debugging. Sometimes can boot XMHF UEFI with DRT and
DMAP, guest OS is either Windows 10 or Windows 11. Thus, the
`CRITICAL_SERVICE_FAILED` error may be unstable. Also, during development the
EFI partition on Dell 7050 once corrupted, not sure whether this contributes
to the blue screen.

For now, going to end this bug.

## Fix

`xmhf64 32d731d8e..a7fb6764b`
* Restore `MSR_IA32_MISC_ENABLE` after SL boot
* Fix tboot bug: should store MSRs as 64-bit integers

