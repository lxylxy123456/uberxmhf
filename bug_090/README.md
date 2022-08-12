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

### Running LHV on Touch

* 32 bit LHV, `LHV_OPT=0x1fd`, failed (stuck at some point)
* 32 bit LHV, `LHV_OPT=0xc`, failed (stuck at some point)
* 32 bit LHV, `LHV_OPT=0x0`, good
* 32 bit LHV, `LHV_OPT=0xd1`, failed (exception on all CPUs)

Tentatively giving up because debugging on Touch is hard. Will do when have
better hardware.

### KVM XMHF KVM stuck at 0x28b8

Now when running KVM XMHF KVM, L2 KVM seems to stuck at `EIP=0x28b8`. L1 XMHF
always prints
```
HPT[3]:xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_get_pmeo:392: EU_CHK( hpt_pmeo_is_present(pmeo)) failed
```

The registers printed by L2 QEMU are:
```
EAX=00000720 EBX=00006726 ECX=00003faa EDX=00000300
ESI=0e0fff67 EDI=000000ac EBP=00006cc2 ESP=00006cb2
EIP=000028b8 EFL=00010046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =b800 000b8000 ffffffff 00809300
CS =c000 000c0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =0000 00000000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=0000000000000000 DR1=0000000000000000 DR2=0000000000000000 DR3=0000000000000000 
DR6=00000000ffff0ff0 DR7=0000000000000400
EFER=0000000000000000
FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80
...
```

The call stack is
```
#0  0x000000001023da7a in hptw_checked_get_pmeo (pmeo=0x1fad8e40 <g_cpustacks+65088>, ctx=0x1029fbc8 <ept02_cache+648>, access_type=2, cpl=HPTW_CPL0, va=753836) at xmhf/src/libbaremetal/libxmhfutil/hptw.c:392
#1  0x0000000010221748 in xmhf_nested_arch_x86vmx_handle_ept02_exit (vcpu=0x1025c9c0 <g_vcpubuffers>, cache_line=0x1029fb80 <ept02_cache+576>, guest2_paddr=753836, qualification=386) at arch/x86/vmx/nested-x86vmx-ept12.c:465
#2  0x000000001020b9bf in xmhf_nested_arch_x86vmx_handle_vmexit (vcpu=0x1025c9c0 <g_vcpubuffers>, r=0x1fad8f78 <g_cpustacks+65400>) at arch/x86/vmx/nested-x86vmx-handler.c:999
#3  0x00000000102087fd in xmhf_parteventhub_arch_x86vmx_intercept_handler (vcpu=0x1025c9c0 <g_vcpubuffers>, r=0x1fad8f78 <g_cpustacks+65400>) at arch/x86/vmx/peh-x86vmx-main.c:1165
#4  0x00000000102064cc in xmhf_parteventhub_arch_x86vmx_entry () at arch/x86/vmx/peh-x86vmx-entry.S:86
#5  0x0000000000000000 in ?? ()
```

The EPT exit information is
* `guest2_paddr = 0xb80ac`
* `qualification = 0x182`
* `r->edi = 0xac`, so likely the access is to `ES:DI`

The EPT entry is `0x00300f80000b82c6`. Where the write bit is set but the read
bit is not. However, `hptw_checked_get_pmeo()` only checks the read bit, which
causes the problem. As a result, XMHF will always perform `L2 -> L0 -> L1` EPT
exits, even though it should be `L2 -> L0 -> L2`. Fixed in `xmhf64 ab9768a9f`.

However, XMHF halt on the same instruction because the physical address of the
EPT entry is wrong. Most other EPT entries look like `0x0010000020XXXc77`, but
this one is far beyond 4G memory limit. Is XMHF expecting an EPT
misconfiguration VMEXIT?

```
0x360b000:	0x0010000020e00c77	0x0000000000000000
0x360b010:	0x0000000000000000	0x0000000000000000
0x360b020:	0x0000000000000000	0x0000000000000000
0x360b030:	0x0010000020e06c77	0x0000000000000000
0x360b040:	0x0000000000000000	0x0000000000000000
0x360b5b0:	0x0000000000000000	0x0000000000000000
0x360b5c0:	0x00300f80000b82c6	0x0000000000000000
0x360b5d0:	0x0000000000000000	0x0000000000000000
0x360b5f0:	0x0000000000000000	0x0000000000000000
0x360b600:	0x0010000020ec0c77	0x0010000020ec1c77
0x360b610:	0x0010000020ec2c77	0x0010000020ec3c77
0x360b620:	0x0010000020ec4c77	0x0010000020ec5c77
0x360b630:	0x0010000020ec6c77	0x0010000020ec7c77
0x360b640:	0x0010000020ec8c77	0x0010000020ec9c77
0x360b650:	0x0000000000000000	0x0000000000000000
0x360b740:	0x0000000000000000	0x0000000000000000
0x360b750:	0x0010000020eeac77	0x0010000020eebc77
0x360b760:	0x0010000020eecc77	0x0000000000000000
0x360b770:	0x0010000020eeec77	0x0000000000000000
0x360b780:	0x0010000020ef0c77	0x0010000020ef1c77
0x360b790:	0x0010000020ef2c77	0x0010000020ef3c77
0x360b7a0:	0x0010000020ef4c77	0x0010000020ef5c77
0x360b7b0:	0x0010000020ef6c77	0x0010000020ef7c77
0x360b7c0:	0x0010000020ef8c77	0x0000000000000000
0x360b7d0:	0x0000000000000000	0x0000000000000000
0x360b7e0:	0x0010000020efcc77	0x0010000020efdc77
0x360b7f0:	0x0000000000000000	0x0000000000000000
```

By reading KVM's `arch/x86/kvm/vmx/vmx.c`, we can see that KVM's use of `-WX`
bits in EPT is intended to produce a EPT misconfiguration. So we should
generate the EPT misconfiguration instead. In this case we have to compute by
software. Letting the hardware generate the EPT misconfiguration and forward
the VMEXIT is hard / impossible.

```c
4287  static void ept_set_mmio_spte_mask(void)
4288  {
4289  	/*
4290  	 * EPT Misconfigurations can be generated if the value of bits 2:0
4291  	 * of an EPT paging-structure entry is 110b (write/execute).
4292  	 */
4293  	kvm_mmu_set_mmio_spte_mask(VMX_EPT_MISCONFIG_WX_VALUE, 0);
4294  }
```

Intel v3 "27.2.3.1 EPT Misconfigurations" documents situations where EPT
misconfiguration should be raised. For now we only implement the `R=0 && W=1`
case.

Intel v3 "26.2.1 Basic VM-Exit Information" shows that EPT misconfiguration
exit needs to set guest physical address VMCS field. The exit qualification
should be cleared.

Fixed in `xmhf64-nest b09f64d9e`. After this commit, SeaBIOS can run
successfully when there are no disks.

TODO: test booting something

