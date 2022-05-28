# XMHF cannot boot Windows 10 on QEMU due to triple fault

## Scope
* bad: x64 and x86 Windows 10, x64 and x86 XMHF, QEMU
* bad: Windows 10, HP (but another bug)

## Behavior
See `bug_027` "Behavior" section for logs

After selecting XMHF and then Windows 10, see triple fault.
x86 and x64 behavior are the same.
```
CPU(0x00): Unhandled intercept: 0x00000002
	CPU(0x00): EFLAGS=0x00000046
	SS:ESP =0xda00:0x000e97d4
	CS:EIP =0xf000:0x000f7d0b
	IDTR base:limit=0x000f615e:0x0000
	GDTR base:limit=0x000f6120:0x0037
```

For `CS:EIP` above, the result is when using `qemu-system-x86_64`. If using
`qemu-system-i386`, the result is `0x0008:0x000f7d0b`.

## Debugging

First debug x86 Windows 10 on x86 XMHF.

Using GDB, see that CR0 = 0x20, CR4 = 0x2000, `CS:EIP = 0x0008:0x000f7d0b`,

`0xf7d0a` is `hlt`. We need to know execution history. First print all
intercepts. Git is `b2fd96af9`, serial is `20220113140640`. Using
`grep Intercept results/20220113140640 | uniq -c`, can see that for intercept
18 (VMCALL, i.e. BIOS e820), the first 9 occurrences are from GRUB, then 162
occurrences are from Windows. However, the last occurrence is not e820? Likely
AMT did not print anything and returned, and then guest halted.
```
      1 CPU(0x00): Intercept 10 @ 0x0008:0x00009321 0x00000000
      9 CPU(0x00): Intercept 18 @ 0x0040:0x000000ac
      1 CPU(0x00): Intercept 10 @ 0x0008:0x00009ae7 0x00000001
      1 CPU(0x00): Intercept 10 @ 0x0008:0x00009af5 0x00000000
      1 CPU(0x00): Intercept 10 @ 0x0008:0x00009b36 0x00000000
      1 CPU(0x00): Intercept 10 @ 0x0008:0x00009b7b 0x00000000
     72 CPU(0x00): Intercept 10 @ 0x0008:0x00009a95 0x00000000
    163 CPU(0x00): Intercept 18 @ 0x0040:0x000000ac
      1 CPU(0x00): Intercept 2 @ 0x0008:0x000f7d0b
```

Now we need to be able to print EIP when guest calls E820.
`_vmx_int15_initializehook` shows how the VMCALL and IRET is set up. We just
need to read EIP from stack.

Git `1c99e1c2a`, serial `20220113145440`

Can see that the last interrupt is 0x2400, instead of 0xe820.
(good ref: <http://www.ctyme.com/intr/int-15.htm>)

For all the e820 intercepts, the `int $0x15` comes from `EIP = 0x9102`, and
then CS:EIP jumps to `0x40:0xac`, which is VMCALL. Then at `0x40:0xaf` perform
IRET, back to `EIP = 0x9104`. These are real mode code.

Since the EIPs are all the same, I think the 162 calls to E820 are also from
GRUB. For Windows 10 it is 9 + 162, for Linux it is 9 + 18, for Windows XP it
is also 9 + 162. Also for Windows XP there is a call to int15 EAX=0x2400:
`20220113145917`. So this is likely not the cause.

`-d int` option in QEMU does not help, because this option does not work in
KVM.

### Booting Windows 10 in QEMU, without XMHF, without PAE

Using `qemu-system-i386`,
* `-m 512M -cpu Haswell,vmx=yes --enable-kvm` works well (can completely boot).
* `-m 512M -cpu Haswell,vmx=yes` will reboot (same as triple fault behavior)
	* Using `-d cpu_reset` can see that there is a triple fault
	* Also helpful: `-no-reboot -no-shutdown`
	* But this triple fault it in protected mode with paging, EIP = 0x007e3a0a
* `-m 512M -cpu Haswell --enable-kvm` works well
* `-m 512M -cpu Haswell` will reboot (same as triple fault behavior)
* If remove the Linux disk (just boot Windows), successful

May need to analyze the difference between KVM and TCG's supported features.
There is a list of features printed at the start of booting.

KVM:
```
qemu-system-i386: warning: host doesn't support requested feature: CPUID.07H:EBX.hle [bit 4]
qemu-system-i386: warning: host doesn't support requested feature: CPUID.07H:EBX.rtm [bit 11]
qemu-system-i386: warning: host doesn't support requested feature: CPUID.80000001H:EDX.lm [bit 29]
```

TCG:
```
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.fma [bit 12]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.pcid [bit 17]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.x2apic [bit 21]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.tsc-deadline [bit 24]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.avx [bit 28]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.f16c [bit 29]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.hle [bit 4]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.avx2 [bit 5]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.invpcid [bit 10]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.rtm [bit 11]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.80000001H:EDX.syscall [bit 11]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.80000001H:EDX.lm [bit 29]
```

Difference:
```
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.fma [bit 12]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.pcid [bit 17]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.x2apic [bit 21]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.tsc-deadline [bit 24]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.avx [bit 28]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.01H:ECX.f16c [bit 29]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.avx2 [bit 5]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.07H:EBX.invpcid [bit 10]
qemu-system-i386: warning: TCG doesn't support requested feature: CPUID.80000001H:EDX.syscall [bit 11]
```

Experiments:
* `-m 512M --enable-kvm -cpu Haswell,x2apic=no`: can show Windows logo
* `-m 512M --enable-kvm -cpu Haswell,x2apic=no,avx=no,avx2=no,f16c=no,fma=no,pcid=no,tsc-deadline=no`: can show Windows logo
* `-m 512M --enable-kvm -cpu Haswell,x2apic=no,avx=no,avx2=no,f16c=no,fma=no,pcid=no,tsc-deadline=no,invpcid=no,syscall=no`: can show Windows logo, can boot
* `-m 512M -cpu Haswell,x2apic=no,avx=no,avx2=no,f16c=no,fma=no,pcid=no,tsc-deadline=no,invpcid=no,syscall=no`: triple fault

Looks like the problem is not related to features.

Even after running `update-grub` in Debian, still the same behavior.

This should be another bug with Windows 10 / grub / QEMU. Since the EIP of this
triple fault is different from XMHF's, ignore for now.

If remove `vmx=yes` in KVM's setting, can still boot Windows 10. Also checked
Windows Security, Hyper-V, and Device Guard. Looks like VMX related features
not enabled.

### Continue debugging

Set a break point at the int15 EAX=0x2400 intercept, and step through using
GDB's `si` command. See `202201131643gdb`

The instruction execution is basically looping:
```
   0x7d47:      pop    %cx
   0x7d45:      int    $0x13
   0xfa6f9:     rep insl (%dx),%es:(%di)
   0xfd3d9:     iret   
   0x9fbd9:     push   %ebp
   0x9fbe3:     iret   
   0x7d47:      pop    %cx
```

The normal execution sequence is:
```
   0x7d69:      ret    
   0x7cd2:      sub    %eax,%ecx
   0x7cd4:      ja     0x7cc5
   0x7cc5:      add    (%esi),%edx
   0x7cc9:      mov    %edx,%es
```

But the last iteration is:
```
   0x7d69:      ret    
   0x7cd2:      sub    %eax,%ecx
   0x7cd4:      ja     0x7cc5
   0x7cd6:      mov    $0x1acdbb00,%eax
   0x7cd9:      int    $0x1a
   0xf7d8b:     add    $0x24,%al
```

We should try to break at `0x7cd6` (`0x07c0:0x000000d6`). Also note that
`0x7c00` is the first sector of the first NTFS partition if booting from GRUB
(see `bug_033`), so we know what we are debugging.

The unassembly is in `real1.s` and `real1.s.16`.

### Sectors loaded

When Windows is started from GRUB chain loading, set a break point during GRUB:
```
hb *0x7c00
c
dump binary memory /tmp/a.img 0x7c00 0x8000
```

Can see that the first NTFS sector is loaded to `0x7c00`.

When Windows is started from BIOS, use QEMU's `-S` and set break point. The
first MBR sector is loaded to `0x7c00`. Deassembly in `real2.s` and
`real2.s.16`. The MBR sector will relocate itself to `0x600` load the NTFS's
first sector to `0x7c00`. If press `c`, will break at NTFS's first sector
(same as starting from GRUB). The registers at that time are (can be considered
Windows official?) (also in `regs_win_mbr.txt`):
```
eax            0x23                35
ecx            0xffff              65535
edx            0x80                128
ebx            0xaa55              43605
esp            0x7c00              0x7c00
ebp            0x7be               0x7be
esi            0x7e00              32256
edi            0x800               2048
eip            0x7c00              0x7c00
eflags         0x246               [ IOPL=0 IF ZF PF ]
cs             0x0                 0
ss             0x0                 0
ds             0x0                 0
es             0x0                 0
fs             0x0                 0
gs             0x0                 0
fs_base        0x0                 0
gs_base        0x0                 0
k_gs_base      0x0                 0
cr0            0x10                [ ET ]
cr2            0x0                 0
cr3            0x0                 [ PDBR=0 PCID=0 ]
cr4            0x0                 [ ]
cr8            0x0                 0
efer           0x0                 [ ]
mxcsr          0x1f80              [ IM DM ZM OM UM PM ]
```

When compared with SeaBIOS's setup (see also `bug_033`), most GPRs are
different. Only EDX and EIP and EFLAGS.IF are the same. ESP are all set with
some non-zero values. (`regs_seabios.txt`)

When compared with GRUB's setup, GRUB disabled EFLAGS.IF. (`regs_grub.txt`)

Now we survey XMHF's behavior. Right after CPU 0 VMENTRY, see `regs_xmhf.txt`.
Compare with `regs_seabios.txt`, the only difference that may matter is that
ESP in XMHF is set to 0. EAX should be arbitrary. CR0 and CR4 look different
in GDB due to VMX shadowing etc.

In XMHF, after selecting "Windows" in GRUB and jumping to NTFS's first sector,
see `regs_xmhf_grub.txt`. It is the same as `regs_grub.txt`, other than CR0 and
CR4.

Also tried to change XMHF's GRUB entry from `module --nounzip (hd0)+1` to
`module --nounzip (hd1)+1` (i.e. load Windows' boot loader instead of GRUB),
and add `drivemap -s (hd0) (hd1)`, but not successful (still triple fault).

Looks like we didn't find useful information here. Need to dive into NTFS asm.

### QEMU bug

When trying to debug the first MBR sector of Windows, found another assertion
error in QEMU / KVM:

1. Run:
```
qemu-system-x86_64 --drive media=disk,file=qemu/windows/win10x86-a.qcow2,index=1 --enable-kvm -s -S
```
2. Connect GDB, run:
```
hb *0x7c00
c
si 1000000000
```
3. See:
```
qemu-system-x86_64: ../hw/core/cpu-sysemu.c:77: cpu_asidx_from_attrs: Assertion `ret < cpu->num_ases && ret >= 0' failed.
```

### Continue using GDB

Git at `012ab700e`. Use the following to GDB break at int15 EAX=0x2400, then
break at `0x7cd6`.
```
hb arch/x86/vmx/peh-x86vmx-main.c:622
c
hb *0x7cd6
hb *0xd6
c
```

(See `real1.s.16`), `0x7cd9` is `int    $0x1a`. QEMU is using SeaBIOS. In
SeaBIOS source code, `handle_1a()` handles `int $0x1a`. Windows' request is
AX = 0xbb00, looks like it should be `handle_1aXX()` (unsupported).

The IDT of the virtual machine is strange. It is not 8-byte aligned, and the
limit field is 0.

At the `arch/x86/vmx/peh-x86vmx-main.c:622` break point, IDTR is `0x0:0x3ff`.
At triple fault, IDTR is `0xf615e:0x0`.

In QEMU monitor, can use `info registers` to show more registers (including
GDT). Ref: <https://www.qemu.org/docs/master/system/monitor.html>.

Regardless whether using GDB, at `0x7cd6`, QEMU's `info registers` shows:
```
GDT=     00008160 00000028
IDT=     00000000 000003ff
```

Start of incorrect content

<del>So looks like IDT changed after the `int $0x1a` instruction. Using the current
IDT to calculate, the descriptor for `0x1a` is:</del>
```
(gdb) x/gx (0x1a * 8)
0xd0:	0xf000ff53f000ff53
(gdb) 
```
<del>This is the same for all IDT values in `0x80 - 0xf8`. A little bit strange.</del>

<del>Using `gdt.py`, this is</del>
```
Segment Limit 0xff53	Base Address 0xf053f000	Type=0xf S=1 DPL=3 P=1 AVL=0 L=0 DB=0 G=0
```
<del>Using Intel's Table 3-2, this should be "32-bit Trap Gate", </del>

End of incorrect content

Actually real mode does not use IDT. Should use IVT. See
<https://wiki.osdev.org/IVT>. Official document is Intel volume 3
"19.1.4 Interrupt and Exception Handling"

With XMHF:
```
(gdb) x/32wx 0
0x0:	0xf000ff53	0xf000ff53	0xf000e2c3	0xf000ff53
0x10:	0xf000ff53	0xf000ff54	0xf000ff53	0xf000ff53
0x20:	0xf000fea5	0xf000e987	0xf000d407	0xf000d407
0x30:	0xf000d407	0xf000d407	0xf000ef57	0xf000d407
0x40:	0xc0005663	0xf000f84d	0x9f7b0000	0x9fbb0000
0x50:	0xf000e739	0x9f7b0008	0xf000e82e	0xf000efd2
0x60:	0xf000d42c	0xf000e6f2	0xf000fe6e	0xf000ff53
0x70:	0xf000ff53	0xf000ff53	0xf0005fbc	0xc0009460
(gdb) x/wx (0x1a * 4)
0x68:	0xf000fe6e
(gdb) 
```

`0xf000fe6e` is `CS:EIP = 0xf000:0xfe6e`. We can break there using
```
hb *0xfe6e
hb *0xffe6e
```

(looks like in GDB when debugging real mode, need to set two hardware
breakpoints: one at EIP and one at CS * 16 + EIP)

The question is: IVT are the same regardless of whether using XMHF or not. But
`_vmx_int15_initializehook()` should have modified the IVT. What's happening?

### Survey IVT

After SeaBIOS jumps to `0x7c00`, IVT is
```
(gdb) x/32wx 0
0x0:	0xf000ff53	0xf000ff53	0xf000e2c3	0xf000ff53
0x10:	0xf000ff53	0xf000ff54	0xf000ff53	0xf000ff53
0x20:	0xf000fea5	0xf000e987	0xf000d407	0xf000d407
0x30:	0xf000d407	0xf000d407	0xf000ef57	0xf000d407
0x40:	0xc0005663	0xf000f84d	0xf000f841	0xf000e3fe
0x50:	0xf000e739	0xf000f859	0xf000e82e	0xf000efd2
0x60:	0xf000d42c	0xf000e6f2	0xf000fe6e	0xf000ff53
0x70:	0xf000ff53	0xf000ff53	0xf0005fbc	0xc0009460
(gdb) 
```

After XMHF's CPU 0 VMLAUNCH, at `0x7c00`, IVT is
```
(gdb) x/32wx 0
0x0:	0xf000ff53	0xf000ff53	0xf000e2c3	0xf000ff53
0x10:	0xf000ff53	0xf000ff54	0xf000ff53	0xf000ff53
0x20:	0xf000fea5	0xf000e987	0xf000d407	0xf000d407
0x30:	0xf000d407	0xf000d407	0xf000ef57	0xf000d407
0x40:	0xc0005663	0xf000f84d	0xf000f841	0xf000e3fe
0x50:	0xf000e739	0x004000ac	0xf000e82e	0xf000efd2
0x60:	0xf000d42c	0xf000e6f2	0xf000fe6e	0xf000ff53
0x70:	0xf000ff53	0xf000ff53	0xf0005fbc	0xc0009460
(gdb) 
```

The only change from SeaBIOS is that `0x15`'s handler is changed to XMHF's.

Without XMHF, the first MBR sector does not chagne IVT. Likely it is not
changed later.

With XMHF, the IVT is changed before the int15 EAX=0x2400. Using print
debugging, see git `23f2c95a3`, can see that the IVT changed after the last
E820 intercept, and before the 0x2400 intercept. Serial in `20220114161044`.
When booting Linux with XMHF, the IVT never changed (serial `20220114161227`).

We need to be able to set break points. Either when XMHF loads GRUB and GRUB
loads NTFS, or watch for change in IVT.

On git `d2b188322`, use gdb script:
```
set pagination off
set confirm off
hb arch/x86/vmx/peh-x86vmx-main.c:623
c
hb *0x7c00
```

After debugging, it looks like int15 EAX=0x2400 happens before break point at
`0x7c00`. So the problem is between SeaBIOS, GRUB, and XMHF. Likely `drivemap`
or something changed the IVT. Now to confirm, we need to set a break point when
without XMHF.

GDB script is:
```
set pagination off
set confirm off
hb *0x7c00
c
# Currently at GRUB
c
# currently at NTFS
```

Run result is
```
0x0000fff0 in ?? ()
Hardware assisted breakpoint 1 at 0x7c00

Thread 1 hit Breakpoint 1, 0x00007c00 in ?? ()
(gdb) c
Continuing.

Thread 1 hit Breakpoint 1, 0x00007c00 in ?? ()
(gdb) x/32wx 0
0x0:	0xf000ff53	0xf000ff53	0xf000e2c3	0xf000ff53
0x10:	0xf000ff53	0xf000ff54	0xf000ff53	0xf000ff53
0x20:	0xf000fea5	0xf000e987	0xf000d407	0xf000d407
0x30:	0xf000d407	0xf000d407	0xf000ef57	0xf000d407
0x40:	0xc0005663	0xf000f84d	0x9f7b0000	0x9fbb0000
0x50:	0xf000e739	0x9f7b0008	0xf000e82e	0xf000efd2
0x60:	0xf000d42c	0xf000e6f2	0xf000fe6e	0xf000ff53
0x70:	0xf000ff53	0xf000ff53	0xf0005fbc	0xc0009460
(gdb) x/i $eip
=> 0x7c00:	jmp    0x7c54
(gdb) x/s $eip
0x7c00:	"\353R\220NTFS    "
(gdb) 
```

We can see that the 4 entries in 0x48 - 0x58 (0x12, 0x13, 0x14, 0x15) are
modified by GRUB.

### Compare behavior

* A: use XMHF to boot grub, and then boot Windows. Run `cmp3a.gdb`, then `si`
     until triple fault. Logs in `cmp3a.txt`.
* B: use GRUB to boot Windows. Run `cmp3b.gdb`, then `si` until QEMU crashes
     (see `ret < cpu->num_ases && ret >= 0`). Logs in `cmp3b.txt`.

Using `diff <(grep ^..........: cmp3a.txt) <(grep ^..........: cmp3b.txt)` and
`diff <(grep ^0x cmp3a.txt) <(grep ^0x cmp3b.txt)`, can see that the
instructions executed are exactly the same. Better to dump the first 1M memory.

Can break at 0xf7d06:
```
hb *0x7d06
hb *0xf7d06
```

And then `si` to `0xf7d08`, and dump the first 1MB of memory. Results in
`202201141723a.img` and `202201141723b.img`. Test another time and results in
`202201141726a.img` and `202201141726b.img`. Note that the results are not
always the same in the same configuration. However, the upper 65536 bytes
(0x0f0000 - 0x100000) are always the same.

See <https://stackoverflow.com/questions/14290879/>, can use objdump on binary
file:
```
objdump -b binary -m i8086 -D 202201141726b.img | grep '^   f'
```

At `0xf7d08`, the register state for both machines are the same, except CR0 and
CR4. I have to suspect that KVM may have a bug in this case.

## I/O access

The instructions that caused the trouble are:
```
   f7cf4:	66 b8 b5 00 00 00    	mov    $0xb5,%eax
   f7cfa:	66 b9 34 12 00 00    	mov    $0x1234,%ecx
   f7d00:	66 bb 0b 7d 0f 00    	mov    $0xf7d0b,%ebx
   f7d06:	e6 b2                	out    %al,$0xb2
   f7d08:	f3 90                	pause  
   f7d0a:	f4                   	hlt    
   f7d0b:	89 f0                	mov    %si,%ax
   f7d0d:	ff d7                	call   *%di
```

In QEMU, using `info mtree`, can see the I/O access is `apm-io`
```
address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io
    ...
    0000000000000092-0000000000000092 (prio 0, i/o): port92
    00000000000000a0-00000000000000a1 (prio 0, i/o): pic
    00000000000000b2-00000000000000b3 (prio 0, i/o): apm-io
    00000000000000c0-00000000000000cf (prio 0, i/o): dma-chan
    00000000000000d0-00000000000000df (prio 0, i/o): dma-cont
    00000000000000f0-00000000000000f0 (prio 0, i/o): ioportF0
    ...
```

APM should stand for "Advanced Power Management". Can we disable it? Doesn't
sound like possible.

Related source files may be:
* <https://github.com/qemu/qemu/blob/master/hw/isa/apm.c>
* <https://github.com/qemu/seabios/blob/master/src/apm.c>

Looks like not easy to disable. Even if we disable it Windows may not boot
correctly.

By debugging the QEMU binary on handling the `out    %al,$0xb2` instruction,
can see that an SMI interrupt is generated. And then the QEMU assertion error
is caused by trying to read memory after GDB's request?

(When debugging QEMU binary, need to ignore SIGUSR1 in GDB. Use
`handle SIGUSR1 noprint nostop`)

### Setting a further break point

In "Compare behavior", B (no XMHF), can set a break point at `0xf7d8d`:
```
Thread 1 hit Breakpoint 6, 0x00007d06 in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d06:	out    %al,$0xb2
0x00007d08 in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d08:	pause  
(gdb) info reg
eax            0xb5                181
ecx            0x1234              4660
edx            0xe97ff             956415
ebx            0xf7d0b             1015051
esp            0xe97d4             0xe97d4
ebp            0xf7d4              0xf7d4
esi            0xe97e8             956392
edi            0xefdca             982474
eip            0x7d08              0x7d08
eflags         0x6                 [ IOPL=0 PF ]
cs             0xf000              61440
ss             0xda00              55808
ds             0xda00              55808
es             0xda00              55808
fs             0x0                 0
gs             0x0                 0
fs_base        0x0                 0
gs_base        0x0                 0
k_gs_base      0x0                 0
cr0            0x10                [ ET ]
cr2            0x0                 0
cr3            0x0                 [ PDBR=0 PCID=0 ]
cr4            0x0                 [ ]
cr8            0x0                 0
efer           0x0                 [ ]
mxcsr          0x1f80              [ IM DM ZM OM UM PM ]
(gdb) hb *0xf7d0b
Hardware assisted breakpoint 8 at 0xf7d0b
(gdb) hb *0x7d0b
Hardware assisted breakpoint 9 at 0x7d0b
(gdb) c
Continuing.

Thread 1 hit Breakpoint 8, 0x000f7d0b in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d8b:	add    $0x24,%al
(gdb) info reg
eax            0xb5                181
ecx            0x1234              4660
edx            0xe97ff             956415
ebx            0xf7d0b             1015051
esp            0xe97d4             0xe97d4
ebp            0xf7d4              0xf7d4
esi            0xe97e8             956392
edi            0xefdca             982474
eip            0xf7d0b             0xf7d0b
eflags         0x46                [ IOPL=0 ZF PF ]
cs             0x8                 8
ss             0x10                16
ds             0x10                16
es             0x10                16
fs             0x10                16
gs             0x10                16
fs_base        0x0                 0
gs_base        0x0                 0
k_gs_base      0x0                 0
cr0            0x11                [ ET PE ]
cr2            0x0                 0
cr3            0x0                 [ PDBR=0 PCID=0 ]
cr4            0x0                 [ ]
cr8            0x0                 0
efer           0x0                 [ ]
mxcsr          0x1f80              [ IM DM ZM OM UM PM ]
(gdb) 
```

Can see that at this point, CS = 8. DS etc and EAX are all updated. Not sure
what's happening.

### Researching on QEMU bug report

Likely related bug reports
* <https://access.redhat.com/solutions/5749121>
	* Mentions "big real mode", and SMM is running in big real mode.
* <https://forum.proxmox.com/threads/qemu-kvm-internal-error-smm-removal-bit-rot.94211/>
	* <https://lists.proxmox.com/pipermail/pve-devel/2021-August/049690.html>
	* Mentions SMM

To continue debugging, is it possible to disable SMM?
Not likely, as Wikipedia says in
<https://en.wikipedia.org/wiki/System_Management_Mode>:
> It is available in all later microprocessors in the x86 architecture.
  [citation needed]

Current guess of the situation:
* Why does debugging fail?
	* QEMU / KVM has a bug in debugging SMM. Probably related to my hardware
	  version.
* Why does XMHF fail?
	* QEMU / KVM has a bug, may be related to the other one.
	* `_vmx_int15_initializehook()` changes code needed by SMM mode, causing
	  undefined behavior.

Next steps
* Read about how VMX handles SMI
* Play with CR0 and CR4 (unlikely)
* Try to remove `_vmx_int15_initializehook` and see what happens
* Try to ignore triple fault and continue anyway, see whether get VMENTRY error
* Try bochs
* Give up and debug on HP only

### SMM and SMI

Intel volume 3 chapter 30 describes SMM. After reading, looks like using the
default SMM treatment should be OK.

SMM code should be at `0x30000 - 0x40000` (`SMBASE`), but from the gdb memory
dump (`20220114172{3,6}{a,b}.img`), this area is 0.

In SeaBIOS source code `smm.c`, a comment says "relocate SMBASE to 0xa0000"

In QEMU's `info mtree`, `00000000000a0000-00000000000bffff` is shown to be both
for VGA and for SMRAM. In GDB's memory dump, looks like the result is VGA.

We guess that the SMM handler address is either 0x38000 or 0xa8000. So break
at these addresses. Using "Compare behavior", B (no XMHF):
```
(gdb) x/i $eip
=> 0x7d08:	popa   
(gdb) p/x $cs
$4 = 0xf000
(gdb) x/i 0xf7d08
   0xf7d08:	pause  
(gdb) info b
Num     Type           Disp Enb Address    What
8       hw breakpoint  keep y   0x000a8000 
9       hw breakpoint  keep y   0x00038000 
(gdb) hb *0xf7d0b
Hardware assisted breakpoint 10 at 0xf7d0b
(gdb) hb *0x7d0b
Hardware assisted breakpoint 11 at 0x7d0b
(gdb) c
Continuing.

Thread 1 received signal SIGTRAP, Trace/breakpoint trap.
1: x/i $cs * 16 + $eip
<error: No registers.>
../../gdb/thread.c:72: internal-error: thread_info* inferior_thread(): Assertion `current_thread_ != nullptr' failed.
A problem internal to GDB has been detected,
further debugging may prove unreliable.

This is a bug, please report it.  For instructions, see:
<https://www.gnu.org/software/gdb/bugs/>.

已放弃 (核心已转储)
```

At the same time QEMU encounters the `ret < cpu->num_ases && ret >= 0`
assertion error.

After more debugging can see that `0x000a8000` is the entry point. So we are
now sure that a lot of things may happen during SMI. However we cannot debug
it easily in QEMU.

### GRUB's implementation of changing IVT

In GRUB source code (branch `master`, commit
`246d69b7ea619fc1e77dcc5960e37aea45a9808c`),
`grub-core/mmap/i386/pc/mmap_helper.S` and `grub-core/mmap/i386/pc/mmap.c` show
how GRUB changes IVT. The old IVT handlers are stored in
`grub_machine_mmaphook_int15offset` and `grub_machine_mmaphook_int15segment`.
Reverting GRUB's changes to the e820 table is simply assigning these values
back to the IVT.

Tried to remove `_vmx_int15_initializehook()`, git is `775fece78`, still see
triple fault.

Tried to revert CR0 and CR4 to settings before `bug_033`. Git are `cb83290ea`
and `51c586520`. However still see triple fault. For `51c586520` when triple
fault, VMCS fields are:
```
(gdb) p/x vcpu->vmcs.control_CR4_mask
$1 = 0x2000
(gdb) p/x vcpu->vmcs.guest_CR4
$2 = 0x2000
(gdb) p/x vcpu->vmcs.control_CR4_shadow 
$3 = 0x2000
(gdb) p/x vcpu->vmcs.control_CR0_shadow 
$4 = 0x20
(gdb) p/x vcpu->vmcs.control_CR0_mask 
$5 = 0x60000020
(gdb) p/x vcpu->vmcs.guest_CR0
$6 = 0x20
(gdb) 
```

Now we try to change the CR0 and CR4 in "Compare behavior", B (no XMHF) using
GDB and see what happens.

```
Thread 1 hit Breakpoint 6, 0x00007d06 in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d06:	out    %al,$0xb2
(gdb) p/x $cr4
$4 = 0x0
(gdb) p/x $cr0
$5 = 0x10
(gdb) p/x $cr0 = 0x30
$6 = 0x30
(gdb) p/x $cr4 = 0x2000
$7 = 0x2000
(gdb) si
0x00007d08 in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d08:	pause  
(gdb) p/x $cr0
$8 = 0x30
(gdb) p/x $cr4
$9 = 0x2000
(gdb) hb *0xf7d0b
Hardware assisted breakpoint 8 at 0xf7d0b
(gdb) hb *0x7d0b
Hardware assisted breakpoint 9 at 0x7d0b
(gdb) c
Continuing.

Thread 1 hit Breakpoint 8, 0x000f7d0b in ?? ()
1: x/i $cs * 16 + $eip
   0xf7d8b:	add    $0x24,%al
(gdb) p/x $cr0
$10 = 0x11
(gdb) p/x $cr4
$11 = 0x0
(gdb) 
```

Can see from above that if change CR0 and CR4 to the same as
"Compare behavior", A, nothing error happens, and CR0 and CR4 get reset,
protected mode is enabled, etc.

### Source of code

I now wonder where the code for `0xf7d0b` comes from. If we break at the first
instruction of NTFS (after GRUB), we can see that the instructions are there.
If we break at the first instruction of GRUB (after BIOS), the instructions
are still there.

When we trace the instructions after Windows calls `int $0x1a`, looks like the
BIOS executes non-trivial handling code and then errors at `0xf7d08`. See
`202201152035gdb`.

Also, when searching "int 1ah bb00" on Google, can see that Windows' BIOS call
is likely related to TPM. e.g.
* <https://zh.osdn.net/projects/openpts/wiki/PlatformBiosInt1AhInfo>
* <https://github.com/egormkn/mbr-boot-manager/blob/master/windows.asm>

So `0xbb00` likely means "TCG Status Check". This makes sense, because TPM
handling likely uses SMM.

The questions now are:
* What is the source for `out    %al,$0xb2`? It should be SeaBIOS, but cannot
  find it easily.
* Where are TPM documentations that define int1a EAX=0xbb00?
* Can we reproduce this bug without Windows?
* Can we disable TPM in Windows?

Now the good news is that we have a workaround: re-compile a BIOS image and
hardcode int1a EAX=0xbb00 to report that TPM does not exist.

### Standard for int 1ah

The standard can be found in
"TCG PC Client Specific Implementation Specification for Conventional BIOS":
* <https://trustedcomputinggroup.org/resource/pc-client-work-group-specific-implementation-specification-for-conventional-bios/>
* <https://www.trustedcomputinggroup.org/wp-content/uploads/TCG_PCClientImplementation_1-21_1_00.pdf>

In chapter 13, it says that AH = 0xbb in general. In "13.7 TCG_StatusCheck",
it says `TCG_PC_TPM_NOT_PRESENT` can be used to return that TPM is not
discoverable. This return code are defined in "14. Error and Return Codes".

Also according to chapter 13, we should be able to use CF = 1 to make Windows
think that TPM BIOS functions are not implemented.

Now maybe we can think of how to capture BIOS calls in XMHF without modifying
memory.

### Reproducing without Windows

This bug can be reproduced without Windows. Simply call int1a EAX=0xbb00.
The intended return result is that AX = 0x23 (`TCG_PC_TPM_NOT_PRESENT`). When
without XMHF, can set a break point at `0xf7d06` and keep stepping. Will then
see QEMU's `ret < cpu->num_ases && ret >= 0` assertion error. See `repr4b.gdb`.

Using the same technique, we can reproduce the triple fault when XMHF is used.
See `repr4a.gdb`.

### Continuing anyway in VMX

If we ignore triple fault, (git `8547a21db`), VM execution will continue (no
VMENTRY error). However, there will then be intercepts at guest
`CS:EIP = 0x0050:0x00000453` forever. Also, when breaking at
`xmhf_parteventhub_arch_x86vmx_intercept_handler()`, cannot access XMHF
variables like VCPU. I think maybe this is code in SMM. For now we should try
to disable TPM or write a fake int1a EAX=0xbb00 handler.

### Disable TPM in Windows

In `tpm.msc`, Windows just says that it cannot find TPM. Looks like there is no
way to prevent Windows from accessing BIOS function to discover TPM.

Ref:
<https://docs.microsoft.com/en-us/windows/security/information-protection/tpm/initialize-and-configure-ownership-of-the-tpm>

### Changing BIOS interrupt handler

Currently, XMHF changes int15 handler by editing IVT and using VMCALL.

Maybe we want to capture interrupts. It is likely managed in
"Pin-Based VM-Execution Controls". However, it only allows setting
"External-interrupt exiting". In Intel volume 3 "6.3 SOURCES OF INTERRUPTS",
it looks like `int $0x15` is software generated interrupt, not external
interrupt.

There is also something called Exception Bitmap. We can try whether setting
a bit here will intercept software interrupts. Git is `a3c731270`, looks like
this will not intercept software interrupt.

So we should still edit IVT and use VMCALL.

### Source for `out    %al,$0xb2` in SeaBIOS

* <https://www.seabios.org/Build_overview>
* <https://www.seabios.org/Debugging>

```
cd /tmp
git clone https://github.com/qemu/seabios/
# commit 6a62e0cb0dfe9cd28b70547dbea5caf76847c3a9
cd seabios
make
```

Using my self-compiled SeaBIOS, QEMU command is
`-bios /tmp/seabios/out/bios.bin`. The exception PC now becomes
`CS:EIP =0x0008:0x000f7d2a`. Can view symbols in `out/rom16.o`

```
cd out
objdump -m i8086 -dS rom16.o
```

`7d2a` is in `__call32()`. For details of objdump see `seabios5.txt`. This
function is defined in `src/stacks.c`.
```c
   237  // Call a 32bit SeaBIOS function from a 16bit SeaBIOS function.
   238  u32 VISIBLE16
   239  __call32(void *func, u32 eax, u32 errret)
   240  {
   241      ASSERT16();
   242      if (CONFIG_CALL32_SMM && GET_GLOBAL(HaveSmmCall32))
   243          return call32_smm(func, eax);
   244      // Jump direclty to 32bit mode - this clobbers the 16bit segment
   245      // selector registers.
   ...  ...
```

Looks like `CONFIG_CALL32_SMM` is defined, and the actual code is in
`call32_smm()`. `call32_smm()` is inlined.
```c
   147  // Call a SeaBIOS C function in 32bit mode using smm trampoline
   148  static u32
   149  call32_smm(void *func, u32 eax)
   150  {
   ...      ...
   155      asm volatile(
   ...          ...
   162          // Transition to 32bit mode, call func, return to 16bit
   163          "  movl $" __stringify(CALL32SMM_CMDID) ", %%eax\n"
   164          "  movl $" __stringify(CALL32SMM_ENTERID) ", %%ecx\n"
   165          "  movl $(" __stringify(BUILD_BIOS_ADDR) " + 1f), %%ebx\n"
   166          "  outb %%al, $" __stringify(PORT_SMI_CMD) "\n"
   167          "  rep; nop\n"
   168          "  hlt\n"
```

Line 167 is the same as `pause`. `PORT_SMI_CMD` is `0x00b2`. `CALL32SMM_CMDID`
is `0xb5`. The SMM handler is in `src/fw/smm.c` (search for `CALL32SMM_CMDID`).

So currently we have some possible workarounds:
1. Disable `CONFIG_CALL32_SMM` when compiling SeaBIOS
2. Use XMHF to change BIOS int 1ah bb00 call
3. <del>Disable the BIOS int 1ah bb00 in Windows</del> (likely not possible)

For now, we think 1 is the best way.

### Recompile SeaBIOS

Run `make menuconfig`, in `Hardware support`, deselect
`System Management Mode (SMM)`. Configuration file is `src/Kconfig` (cannot set
`CALL32_SMM` in graphical mode, but can set `USE_SMM`, which is prerequisite
for `USE_SMM`).

Then run `make`. Compile result is attached as `bios.bin`.

After disabling SMM, x86 Windows 10 on x86 XMHF, x86 QEMU can see the Windows
logo and the loading symbol with 5 dots spinning. Then can login. Can shutdown
successfully.

### Untested ideas

* Fake int1a
	* If XMHF does not have a bug, no need to fix int1a for now.
* Run QEMU images with bochs (do not install from scratch: too slow)
	* Bochs is too slow, so did not test it

## Result

Likely not a bug in XMHF. I think the problem is that KVM does not handle
nested SMM correctly. Windows 10 calls TPM functions, and SeaBIOS uses SMM to
enter 32-bit mode. This causes the problem.

The temporary fix for running on QEMU is to use self-compiled SeaBIOS. If this
bug is reproduced on HP, may need to use fake BIOS handler.
* QEMU command line: `-bios bug_031/bios.bin`

After `bug_036` fixed: I think `bug_036` is a related bug. However when testing
now I cannot reproduce `bug_031`. Maybe related to QEMU update? Currently
QEMU version is 6.1.0, XMHF commit is `5324ae8e8`

At `bug_077`: the bug re-appears, with a different behavior
(`KVM_GET_PIT2 failed: Input/output error`). Maybe Linux / KVM version is more
interesting. Currently `5.17.8-200.fc35.x86_64`.

