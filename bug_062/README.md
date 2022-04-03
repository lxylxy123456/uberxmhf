# Cleanly report error when running amd64 OS in i386 XMHF

## Scope
* i386 XMHF
* Git `0x10000000`

## Behavior
When running x64 OS in x86 XMHF, looks like XMHF will triple fault. Should do
something different.

HP serial looks like
```
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x0000000d, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x0000000e, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x0000000f, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000010, ECX=0xThe system is powered off.
The system is powered on.
```

QEMU serial is

```
[00]: unhandled exception 3 (0x3), halting!
[00]: state dump follows...
[00] CS:EIP 0x0008:0x10205369 with EFLAGS=0x00000042
[00]: VCPU at 0x10242620
[00] EAX=0x0000000d EBX=0x756e6547 ECX=0x6c65746e EDX=0x49656e69
[00] ESI=0x00000000 EDI=0x0000009e EBP=0x01000000 ESP=0x1d03dff0
[00] CS=0x0008, DS=0x0010, ES=0x0010, SS=0x0010
[00] FS=0x0010, GS=0x0010
[00] TR=0x0018
[00]-----stack dump-----
[00]  Stack(0x1d03dff0) -> 0x10205369
[00]  Stack(0x1d03dff4) -> 0x00000008
[00]  Stack(0x1d03dff8) -> 0x00000042
[00]-----end------------
...
```

## Debugging
The problem in QEMU is that VMRESUME fails, then the failure path (`int $0x03`
causes the unhandled exception). So maybe it helps to find a way to break
there.

We can see the problem is `VM entry with invalid host-state field(s)`
```
CPU(0x00): vcpu->vmcs.info_vminstr_error=0x00000008
```

We can brute force and print all intercepts, then compare the last intercept.

The list of intercepts are
```
guest_RIP=0x01672f39
guest_RIP=0x01672f82
guest_RIP=0x01672fa8
guest_RIP=0x01672fb7
guest_RIP=0x01672fcc
guest_RIP=0x01672fda
guest_RIP=0x01672fef
guest_RIP=0x010000ec
guest_RIP=0x010000f2
guest_RIP=0x0009e040
guest_RIP=0x0009e046
guest_RIP=0x03372c79
```

We print more. Git `99a6ee52d`, serial not saved, but the serial output is
split to `20220402232726_*` files, using `split.py`.

I suddenly realized this code in `part-x86vmx.c`:
```
487  #ifdef __AMD64__
488  	vcpu->vmcs.guest_TR_access_rights = 0x8b; //present, 32/64-bit busy TSS
489  #elif defined(__I386__)
490  	// TODO: should be able to use 0x8b, not 0x83
491  	vcpu->vmcs.guest_TR_access_rights = 0x83; //present, 16-bit busy TSS
492  #else /* !defined(__I386__) && !defined(__AMD64__) */
493      #error "Unsupported Arch"
494  #endif /* !defined(__I386__) && !defined(__AMD64__) */
```

This comes from `bug_014`. We should change it.

Git `7ab4bec61`, serial `20220402233225`. We can see that changing TSS does not
work. But changing it is justified.

We analyze the logs more. `20220402233915_12` is a VMENTRY, and when executing
VMRESUME, VMRESUME fails and the error output is `20220402233915_err`. So
the guest changes the state between `20220402233915_11` and
`20220402233915_12`. However, there are a lot of changes. Maybe we can use
monitor trap.

Git `0e163b031`, serial `20220403000839`. Now we can make the error appear
earlier. Running `diff 20220403000839_{21,22}` shows the difference.

The differences are:
* CR0.PG is set
* VMENTRY control bit 9 (IA-32e mode guest) is set

The code in Linux is:
```
   0x9e040:	rdmsr  
   0x9e042:	bts    $0x8,%eax
   0x9e046:	wrmsr  
   0x9e048:	pop    %rdx
   0x9e049:	pop    %rcx
   0x9e04a:	mov    $0x20,%eax
   0x9e04f:	cmp    $0x0,%edx
   0x9e052:	je     0x9e059
   0x9e054:	or     $0x1000,%eax
   0x9e059:	mov    %rax,%cr4
   0x9e05c:	lea    0x106e(%rcx),%eax
   0x9e062:	push   $0x10
   0x9e064:	push   %rax
   0x9e065:	mov    $0x80000001,%eax
   0x9e06a:	mov    %rax,%cr0
   0x9e06d:	lret   
```

The problem in QEMU can be explained using Intel v3
"25.2.4 Checks Related to Address-Space Size":
> If the logical processor is outside IA-32e mode (if IA32_EFER.LMA = 0) at
> the time of VM entry, the following must hold:
> The “IA-32e mode guest” VM-entry control is 0.

This simply means that an i386 hypervisor cannot run amd64 guest OS.

For QEMU, maybe the best thing we can do is try to detect it and print an
error. This may be user friendly. (A similar thing to do is to detect PAE, but
we will not address it for now).

After fix (git `3bd65eec4`), on HP running amd64 guest on i386 XMHF will stuck.
Not going to address this due to low priority.

## Fix

`69ff2c15c..3bd65eec4`
* Change TR access rights for i386
* In QEMU, print warning if running amd64 guest in i386 XMHF

