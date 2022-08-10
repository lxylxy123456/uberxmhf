# Run KVM XMHF KVM

## Scope
* All configurations
* `xmhf64 35b8a931f`
* `xmhf64-nest 536d95d87`
* `lhv f8111a463`

## Behavior
XMHF should be able to run other hypervisors, like KVM.

## Debugging

The configuration has been done above. Search for "curlftpfs" in `bug_064` and
`bug_078`. The idea is to use `debian11x64-q.qcow2`, which is convenient
because it has QEMU installed. Compared to the configuration in `bug_078`, just
add an XMHF image before `debian11x64-q.qcow2`.

However, the problem happens earlier, since it looks like KVM is not able to
start VMX operations. A simpler way to reproduce is:

```
./bios-qemu.sh --gdb 1234 -d build64 +1 -d debian11x64-q -t -smp 1 -m 1G --ssh 1235
(ssh ... -A -X)
qemu-system-x86_64 -enable-kvm
```

The following error message is seen on QEMU's output:
```
KVM: entry failed, hardware error 0x0
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=00000000 EFL=00000000 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 00000000 00008000
CS =0000 00000000 00000000 00008000
SS =0000 00000000 00000000 00008000
DS =0000 00000000 00000000 00008000
FS =0000 00000000 00000000 00008000
GS =0000 00000000 00000000 00008000
LDT=0000 00000000 00000000 00008000
TR =0000 00000000 00000000 00008000
GDT=     00000000 00000000
IDT=     00000000 00000000
CR0=60000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=0000000000000000 DR1=0000000000000000 DR2=0000000000000000 DR3=0000000000000000 
DR6=00000000ffff0ff0 DR7=0000000000000400
EFER=0000000000000000
Code=<00> 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

For output in dmesg, see `kvm1.txt`

The following output is seen in XMHF.
```
Found in xcph table
CPU(0x00): CS:RIP=0x0010:0xffffffffa8c3dcc2 MOV CR4, xx
MOV TO CR4 (flush TLB?), current=0x001726f0, proposed=0x001726f0
Found in xcph table
```

From dmesg output, looks like the first error is failure of VMPTRLD. We should
debug XMHF to see why this instruction fails.

Debugging XMHF is simple. Just set a break point at `_vmx_nested_vm_fail()` and
similar functions. We can see that the problem happens because when KVM runs
VMPTRLD, the "VMCS revision identifier" of the VMCS is 0.

From Intel v3 "29.3 VMX INSTRUCTIONS", this should not be allowed. So at this
point it looks like KVM is wrong.

By reading KVM source code, we can guess the call stack

```
vmx_vcpu_load_vmcs
	loaded_vmcs_clear
		...
			VMCLEAR
	vmcs_load
		vmx_asm1
			VMPTRLD
```

Currently I have two theories
* Since XMHF prints "Found in xcph table" twice, we check the MSR read / write
  history. Maybe in RDMSR, XMHF gave KVM the incorrect result
* XMHF clears the memory in VMCLEAR, but maybe KVM writes the revision
  identifier first and then VMCLEAR

It turns out that the second one is correct. Now the question is whether KVM
or XMHF violates the spec.

It looks like XMHF violates the spec. Before, my understanding is that VMCLEAR
clears the memory of VMCS. However, the correct behavior is to write the
information in VMCS from "cache" to memory. A future problem is that XMHF
does not currently support running one VMCS on multiple CPUs (in
non-overlapping time). See Intel v3
"23.11.1 Software Use of Virtual-Machine Control Structures"
> No VMCS should ever be active on more than one logical processor. If a VMCS
> is to be “migrated” from one logical
> processor to another, the first logical processor should execute VMCLEAR for
> the VMCS (to make it inactive on that
> logical processor and to ensure that all VMCS data are in memory) before the
> other logical processor executes
> VMPTRLD for the VMCS (to make it active on the second logical processor).1 A
> VMCS that is made active on more
> than one logical processor may become corrupted (see below).

We can verify KVM's behavior by modifying LHV a little bit in
`lhv-dev 73dcc08bf`. Before VMCLEAR, only `VMCS[0] = 0x11e57ed0`. However,
after VMCLEAR, many more fields are present.

Currently XMHF's behavior is to clear all memory as possible. However, we
should change this behavior to
* VMCLEAR: if clearing active VMCS, write VMCS12 content from XMHF back to
  guest
* VMPTRLD: load XMHF's VMCS12 content from guest memory, not initialize with 0

Basically fixed in `xmhf64-nest 89d5cdf31`. 

### Possible KVM bug

The next step is to write a test case in LHV for VMCLEAR and VMPTRLD behavior.
In `lhv-dev 43efd47a3..53c8215e7`, we try to remove `vmcs_load()`, but realize
that KVM seems to have a bug.

The list of events happened are:
1. LHV starts, write VPID = 0xb
2. VMCLEAR, then VMPTRLD, see VPID = 0xb
3. Write VPID = 0x15
4. VMCLEAR, then VMPTRLD, see VPID = 0xb (wrong)

Actually, this is LHV's problem. The test executes VMXOFF without VMCLEAR.
However, Intel v3 "23.11.1 Software Use of Virtual-Machine Control Structures"
says:
> software should execute VMCLEAR for that VMCS before executing the VMXOFF
> instruction or ...

Fixed LHV in `lhv 43cdec2f3` and `lhv-dev 2cdbe294a`. Then implemented test in
`lhv da2c97ffd`. Found that `xmhf64-nest 89d5cdf31` does not pass the test,
because it does not take shadow VMCS into account. Fixed in
`xmhf64-nest 217e5290f`.

### Running LHV in Bochs

I realized that the latest LHV and configuration does not run in Bochs. The
behavior is that CPU 0 prints
```
Guest: interrupt / exception vector 39

Fatal: Halting! Condition '0 && "Guest: unknown interrupt / exception!\n"' failed, line 518, file lhv-guest.c
```

* `LHV_OPT=0x0`, SMP 2, good
* `LHV_OPT=0x80`, SMP 2, good
* `LHV_OPT=0x1fc`, SMP 2, bad
* `LHV_OPT=0x40`, SMP 2, good
* `LHV_OPT=0x1fc`, SMP 1, bad
* `LHV_OPT=0xfc`, SMP 1, bad
* `LHV_OPT=0x7c`, SMP 1, bad
* `LHV_OPT=0x3c`, SMP 1, bad
* `LHV_OPT=0x1c`, SMP 1, bad
* `LHV_OPT=0xc`, SMP 1, bad
* `LHV_OPT=0x4`, SMP 1, good
* `LHV_OPT=0xc2`, SMP 1, good
* `LHV_OPT=0x2c0`, SMP 1, bad 2 (different from "bad" in that the exception
  happens in host mode, not guest mode
* `LHV_OPT=0x200`, SMP 1, bad 2

May be related: "4.1.8 The Mysterious Exception 0x27, aka IRQ 7" in 15605
`kernel.pdf`. See also
<https://en.wikipedia.org/wiki/Intel_8259#Spurious_interrupts>.
Now we start modifying LHV code.

In `lhv-timer.c` function `timer_init()`, looks like the problem happens at the
HLT instruction below. However if `printf("D\n");` below is uncommented, the
problem disappears.
```
			printf("A\n");
			outb(TIMER_ONE_SHOT, TIMER_MODE_IO_PORT);
			outb((u8)(1), TIMER_PERIOD_IO_PORT);
			outb((u8)(0), TIMER_PERIOD_IO_PORT);
			// printf("D\n");
			asm volatile ("sti; hlt; cli");
			printf("E\n");
```

Maybe we can try reproducing this problem in 15605's environment. Actually it
is easy. Just replace `init-x86-i386.bin` with `kernel` and create XMHF GRUB
image. However, we cannot easily reproduce it in 15605's environment. Also,
we are not sure whether Bochs or LHV is buggy.

For now I am going to workaround by handling this interrupt. Implemented in
`lhv d6381993b`. Looks like the IRQ 7 only happens once, which is good.
According to <https://www.reddit.com/r/osdev/comments/5qqnkq/comment/dd1qfx9/>,
we do not send EOIs in this case.

Also according to <https://forum.osdev.org/viewtopic.php?f=1&t=56009>, maybe
the PIT is just ticking too fast on Bochs.

Another problem with Bochs is that MTRR tests do not work. Worked around in
`lhv d6381993b..e31d4b4bc`. The problem is that Bochs does not have machine
check MSRs.

TODO: run LHV in Touch, and XMHF LHV
TODO: continue testing KVM XMHF KVM

