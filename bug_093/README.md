# Test other hypervisors

## Scope
* All configurations, focus on amd64
* `xmhf64 3875e68b1`
* `xmhf64-nest 35ef22bcd`
* `lhv 49258660f`

## Behavior
Currently only KVM has run in XMHF as a hypervisor. Should try other
hypervisors.

## Debugging

Planned hypervisor to try on Linux
* Virtual Box
* VMware
* Xen

### Setting up VirtualBox
Following <https://wiki.debian.org/VirtualBox> to install VirtualBox on Debian.

```sh
sudo apt install fasttrack-archive-keyring
sudo vi /etc/apt/sources.list
	deb https://fasttrack.debian.net/debian-fasttrack/ bullseye-fasttrack main contrib
	deb https://fasttrack.debian.net/debian-fasttrack/ bullseye-backports-staging main contrib
sudo apt-get update
sudo apt-get install virtualbox
# sudo apt install virtualbox-ext-pack
# restart
```

See the following error. Giving up and trying another way.

```
$ VBoxManage list vms
WARNING: The character device /dev/vboxdrv does not exist.
	 Please install the virtualbox-dkms package and the appropriate
	 headers, most likely linux-headers-amd64.

	 You will not be able to start VMs until this problem is fixed.
$ 
```

Removed virtualbox from previous installation, follow
<https://www.virtualbox.org/wiki/Linux_Downloads> now.

```sh
sudo apt-get install gpg
sudo vi /etc/apt/sources.list
	deb [arch=amd64 signed-by=/usr/share/keyrings/oracle-virtualbox-2016.gpg] https://download.virtualbox.org/virtualbox/debian bullseye contrib
wget -O- https://www.virtualbox.org/download/oracle_vbox_2016.asc | sudo gpg --dearmor --yes --output /usr/share/keyrings/oracle-virtualbox-2016.gpg
sudo apt-get update
sudo apt-get install virtualbox-6.1
sudo /sbin/vboxconfig
	# May be asked to install linux-headers-amd64 linux-headers-5.10.0-10-amd64
	# Need to update kernel version
VBoxManage list vms
	# Should not see error
```

### VirtualBox i386 Debian vmentry failure Problem

When trying to boot debian11x86, another problem happens in XMHF:
"Debug: guest hypervisor VM-entry failure."

This problem is also reproducible when trying to run 15605 p3 kernel in
virtualbox. Due to GCC version problems, patch Makefile with the following
before compile.

```diff
diff --git a/Makefile b/Makefile
index 00f0ccf..04baff3 100644
--- a/Makefile
+++ b/Makefile
@@ -187,11 +187,11 @@ CFLAGS_COMMON = -nostdinc \
        -fcf-protection=none \
        --std=gnu99 \
        -D__STDC_NO_ATOMICS__ \
-       -Wall -Werror
+       -Wall # -Werror
 
 CFLAGS_GCC = $(CFLAGS_COMMON) \
        -fno-aggressive-loop-optimizations \
-       -gstabs+ -gstrict-dwarf -O0 -m32
+       -g -gstrict-dwarf -O0 -m32
 
 CFLAGS_CLANG = $(CFLAGS_COMMON) \
        -gdwarf-3 -gstrict-dwarf -Og -m32 -fno-addrsig -march=i386 \
```

Now try to make LHV reproduce this problem
* `lhv-dev fa7d55af6`: LHV do not enter guest mode, not reproducible. However
  timer interrupts (APIC and PIT), keyboard interrupts, and console are good.
* `lhv-dev fdac0ce2b`: LHV enter host user mode, wait forever, timer interrupts
  work, not reproducible.

Now focus on p3 kernel. The VMENTRY failure log is in `entry_fail1.txt`. A
normal VMCS (during BIOS?) look like `sample_vmcs2.txt`. Compare using:
```sh
diff <(cut -d @ -f 1,3 sample_vmcs2.txt | cut -d : -f 2) \
	<(grep failure02 entry_fail1.txt | cut -d @ -f 1,3 | cut -d : -f 2)
```

Note that `info_vmexit_reason = 0x80000022`, which is "34VM-entry failure due
to MSR loading.". This is not the usual "VM-entry failure due to invalid guest
state." Now we need to print MSR data structures.

Git `xmhf64-nest-dev 028a7fecc`, serial `20220919235746`. We have printed all
VMEXITs and some MSRs. From `0x0010a7a0`, the control is transferred to Pebbles
kernel. Can see that `0x00100c65` are EPT misconfig VMEXITs in console driver.
`0x0010cccc` are IO VMEXITs in `outb()`.

Then the following VMEXITs are due to CRx access. The first is CR3, second is
CR0. Looks like VirtualBox emulated the second set CR0 operation, and the
VMENTRY caused the entry failure.
```
CPU(0x05): nested vmexit  0x0010cd17 28
CPU(0x05): nested vmexit  0x0010ccfa 28
```

The asm in pebbles kernel looks like
```
   0x10ccf6 <set_cr0>:	mov    0x4(%esp),%eax
   0x10ccfa <set_cr0+4>:	mov    %eax,%cr0
   0x10ccfd <set_cr0+7>:	pushw  %cs
   0x10ccff <set_cr0+9>:	push   $0x10cd07
   0x10cd04 <set_cr0+14>:	ljmp   *(%esp)
```

The relevant SDM chapter is Intel v3 "25.4 LOADING MSRS". This footnote looks
interesting:
> 1. If CR0.PG = 1, WRMSR to the IA32_EFER MSR causes a general-protection
> exception if it would modify the LME bit. If VM entry has
> established CR0.PG = 1, the IA32_EFER MSR should not be included in the
> VM-entry MSR-load area for the purpose of modifying the
> LME bit.

Current directions are
* Print state before and after `0x0010ccfa`, print more MSRs
* Print KVM behavior at `0x0010ccfa` and compare

Double check: KVM does not have this problem when running Pebbles kernel.

In `xmhf64-nest-dev 280cb659f`, print full information before and after set
CR0. Results for KVM and VirtualBox are in `compare_3_kvm.txt` and
`compare_3_vb.txt`. Note that KVM does not have after set CR0 because setting
CR0 succeeds. Compare using
```sh
diff <(grep before compare_3_vb.txt | cut -d @ -f 1,3) \
     <(grep after  compare_3_vb.txt | cut -d @ -f 1,3)
diff <(grep before compare_3_vb.txt  | cut -d : -f 2-) \
     <(grep before compare_3_kvm.txt | cut -d : -f 2-)		# not used
```

The exit qualification is 1, which indicates the first entry causes the problem
(`MSR_EFER`).

Other interesting changes made by VirtualBox are

|Field                  |before VMCS12|after VMCS12|before VMCS02|after VMCS02|
|-----------------------|-------------|------------|-------------|------------|
|`control_VMX_cpu_based`|0xb5a0fdfa   |0xb5a07dfe  |0xb5a0fdfa   |0xb5a07dfe  |
|`control_CR0_shadow`   |0x00000015   |0x80010015  |0x00000015   |0x80010015  |
|`guest_CR0`            |0x00000035   |0x80010035  |0x00000035   |0x80010035  |

For `vmcs02_vmentry_msr_load_area`, when running KVM, `IA32_EFER=0`. However,
when running VirtualBox, `IA32_EFER=0xd01` (looks like all possible bits set).
This is abnormal.

By reading VirtualBox source code `src/VBox/VMM/VMMR0/HMVMXR0.cpp`, search for
`VMX_EXIT_ERR_MSR_LOAD`, can see that this VMEXIT is unexpected. So ideally if
we move VMCS12 to VMCS02, the error should disappear.

In KVM, VMCS12 sets VMENTRY MSR load `IA32_EFER=0`, so VMCS02 also loads
`IA32_EFER=0`. This should be considered the good behavior. However, in Virtual
Box, there are no VMENTRY MSR load fields. So during VMENTRY, the original
state is (PG=1, PAE=0, LME=0). The WRMSR sets LME, which causes #GP as
specified by i3 "4.1.2 Paging-Mode Enabling".

Git `xmhf64-nest-dev 24028d0c5`, results in `compare_4_kvm.txt` and
`compare_4_vb.txt`.

Now, my guess is that VirtualBox is using the "Load IA32_EFER" VMENTRY control
to load EFER, and instead of using MSR load fields. VirtualBox sets the EFER
VMCS field to 0, so it looks like not being used. However, XMHF does not
provide "Load IA32\_EFER" feature, so VirtualBox does not set the "Load
IA32\_EFER" bit, causing the error. If this is true, it should be considered
a bug in VirtaulBox. The correct behavior is to error.

Now we let XMHF to lie and say that "Load IA32\_EFER" feature is provided. Git
`xmhf64-nest-dev e68667516`, result `compare_5_vb.txt`.

However, actually this guess is invalid. VirtualBox's behavior does not change.
In 2540p, `vcpu->vmx_msrs[4]=0x0000ffff000011ff` and
`vcpu->vmx_nested_msrs[10]=0x0000ffff000011fb`, so hardware does not support
"Load IA32\_EFER".

Now we read Intel manual carefully and extract related information
* VMENTRY
	* Check (25.3.1.1)
		* If "IA-32e mode guest" is 1, CR0.PG (31) must be 1, CR4.PAE (5) must
		  be 1
		* If "IA-32e mode guest" is 0, CR4.PCIDE (17) must be 0
		* If "load `IA32_EFER`" is 1, ...
	* Load (25.3.2.1)
		* `IA32_EFER.LMA` = "IA-32e mode guest"
		* If CR0.PG = 1, `IA32_EFER.LME` = "IA-32e mode guest"
* VMEXIT
	* Check (25.2.4)
		* If `IA32_EFER.LMA` = 0, then "host address-space size" = 0,
		  "IA-32e mode guest" = 0
		* If `IA32_EFER.LMA` = 1, then "host address-space size" = 1
	* Load (26.5.1, 26.5.2)
		* CR4.PAE = 1 if "host address-space size" = 1
		* CR4.PCIDE = 0 if "host address-space size" = 0
		* `IA32_EFER.LME` = "host address-space size"
		* `IA32_EFER.LMA` = "host address-space size"
		* CS.D/B ???

Indeed, this problem is caused by not modifying `IA32_EFER` correctly during
nested VMENTRY and VMEXIT. Fixed in `xmhf64-nest 9d8f6ae1e`. After that,
Pebbles kernel can run successfully. debian11x86 can also boot.

For other fields listed above
* No need to worry about most L2 fields, because L2 is guest in both VMCS12 and
  VMCS02
* L1 fields need to be reviewed

Untried ideas
* Will it work if we simply copy VirtualBox's VMCS12 to VMCS02?
* Try running KVM VirtualBox and KVM XMHF VirtualBox

### Review VMCS fields for L1 after nested VMEXIT

Reviewed Intel v3 26.5 "LOADING HOST STATE", modified XMHF code in
`xmhf64-nest-dev 90707ed03..020b702cf` to update L1 state better. Rebased to
`xmhf64-nest 9d8f6ae1e..8de661b8f`.

### VirtualBox amd64 Debian kill init Problem

After trying to boot the Debian netinst image, see Linux kernel panic. The same
happens when trying to boot debian11x64 (QCOW2 image installed using KVM). The
panic message is "not syncing: Attempted to kill init! exit code=0x0000000b".

From the kernel panic message, looks like the init process received #GP
exception, which causes `do_exit()` to be called. This triggers kernel panic
because init cannot exit.

The RIP of init process does not look stable. So I guess the #GP may be caused
by an interrupt. Otherwise, by looking at Linux panic message, looks like most
instructions are accessing memory. Maybe there is something wrong with page
table / EPT?

The good news is that VirtualBox is open source. So ideally there is no
blackbox during debugging.

First, we can verify that the signal received is #GP. The exit code 0xb is
strange, but the call stack shows `asm_exc_general_protection()` is called. For
exception #NP (0xb), `asm_exc_segment_not_present()` will be used.

By printing at every VMEXIT, looks like the exception bitmap after kernel
enters long mode and becomes stable is always `0x00022002`, which captures
`#GP`. So we should be able to see the exception in XMHF log.

In `xmhf64-nest-dev 9c42efd25`, we see a strange pattern
```
...
CPU(0x04): nested vmexit  0xffffffffb0c12050 48
CPU(0x04): nested vmexit  0x7f88f438297c 10
CPU(0x04): nested vmexit  0x7f88f4382258 10
CPU(0x04): nested vmexit  0x7f88f43822b5 10
CPU(0x04): nested vmexit  0x7f88f4382d5b 10
CPU(0x04): nested vmexit  0x7f88f4382d6b 10
CPU(0x04): nested vmexit  0x7f88f4382d92 10
CPU(0x04): nested vmexit  0x7f88f4382db9 10
CPU(0x04): nested vmexit  0xffffffffb0e01007 0: 0x80000b0d
CPU(0x04): nested vmentry 0xffffffffb0e01007 intr_inj=0x80000b0d
```

Looks like Linux enters user mode, then performs some CPUIDs, then at
`0xffffffffb0e01007` gets a #GP. This pattern consistently occurs 5 times,
which is strange because we are expecting only one #GP to kill the init
process.

Also, in VirtualBox log (`~/VirtualBox VMs/*/Logs/VBox.log`), see
`IEM: rdmsr(0x140) -> #GP(0)`. This message is close to when the kernel panic
happens. This message only happens once. The call stack in VirtualBox should be:
```
hmR0VmxExitRdmsr
	IEMExecDecodedRdmsr
		iemCImpl_rdmsr
```

In `xmhf64-nest-dev b9c3d8e54`, manually change `intr_inj=0x80000b0d` to
`0x80000b0b`. Can see that Linux panic message also changes. So the kernel
panic happens in kernel code, though Linux prints mainly user space registers.
By further changing, looks like the last exception of the 5 will be reported in
the kernel panic message.

As for the RDMSR to 0x140, XMHF log shows that it should be unrelated. It
happens before the 5 tries to spawn init.

Also, why does Linux try to spawn init 5 times? Probably because it is the
standard to try spawning a service 5 times. init is systemd, which is a
service. e.g. <https://openwrt.org/docs/guide-developer/procd-init-scripts>:
> if process dies sooner than respawn\_threshold, it is considered crashed and
> after 5 retries the service is stopped

If without KASLR, the exception happens in `0xffffffff81a01007`. The symbol is
`native_irq_return_iret`. The Linux source code is in
`arch/x86/entry/entry_64.S`, which looks like
```
   0xffffffff81a01000 <native_iret>:	testb  $0x4,0x20(%rsp)
   0xffffffff81a01005 <native_iret+5>:	jne    0xffffffff81a01009 <native_irq_return_ldt>
   0xffffffff81a01007 <native_irq_return_iret>:	iretq  
   0xffffffff81a01009 <native_irq_return_ldt>:	push   %rdi
   0xffffffff81a0100a <native_irq_return_ldt+1>:	swapgs 
   ...
```

So now the question is why iret causes the exception. We should start
collecting VMCS fields during the VMEXIT.

Git `xmhf64-nest-dev 15f44fd42`, serial `20220925152711`. Notice that the
exception error code is 0x10 instead of 0. This should be helpful.
```
CPU(0x05): (0x4402) :VMCS02:info_vmexit_reason = 0x00000000
CPU(0x05): (0x4404) :VMCS02:info_vmexit_interrupt_information = 0x80000b0d
CPU(0x05): (0x4406) :VMCS02:info_vmexit_interrupt_error_code = 0x00000010
CPU(0x05): (0x4408) :VMCS02:info_IDT_vectoring_information = 0x00000000
CPU(0x05): (0x440a) :VMCS02:info_IDT_vectoring_error_code = 0x00000000
CPU(0x05): (0x440c) :VMCS02:info_vmexit_instruction_length = 0x00000002
```

Intel v2 shows a list of possible causes to this exception, see
`#GP(Selector)`. However, a strange thing is that we are expecting error code
`0x13`, not `0x10`. Linux GDT is defined in
`arch/x86/boot/compressed/head_64.S`.

In git `xmhf64-nest-dev ad9ca3488`, serial `20220925155823`, also print GDT.
Now we follow check the checklist. The Segment descriptor is
`0x00af9b000000ffff`, `gdt.py` gives
```
Segment Limit 0xfffff	Base Address 0x0	Type=0xb S=1 DPL=0 P=1 AVL=0 L=1 DB=0 G=1
```

* If a segment selector index is outside its descriptor table limits.
	* No, `guest_GDTR_limit=0x7f`, but segment selector is `0x13`.
* If a segment descriptor memory address is non-canonical.
	* No, memory address is `0`, which is canonical.
* If the segment descriptor for a code segment does not indicate it is a code
  segment.
	* No, type is 0xb, which is code segment type (0x8-0xf)
* If the proposed new code segment descriptor has both the D-bit and L-bit set.
	* No, D=0
* If the DPL for a nonconforming-code segment is not equal to the RPL of the
  code segment selector.
	* This is probably the cause. Type 0xb means it is nonconforming, DPL=0.
	  However, RPL=3.
		* TODO
* If CPL is greater than the RPL of the code segment selector.
	* No, CPL = 0 and RPL = 3
* If the DPL of a conforming-code segment is greater than the return code
  segment selector RPL.
	* No, DPL = 0 and RPL = 3. Also this is nonconforming
* If the stack segment is not a writable data segment.
	* No, not related to stack
* If the stack segment descriptor DPL is not equal to the RPL of the return
  code segment selector.
	* No, not related to stack
* If the stack segment selector RPL is not equal to the RPL of the return code
  segment selector.
	* No, not related to stack

Using QEMU to quickly experiment, a normal Linux user program uses CS=0x33 and
SS=0x2b. CS descriptor is `0x00affb000000ffff`, `gdt.py` shows:
```
Segment Limit 0xfffff	Base Address 0x0	Type=0xb S=1 DPL=3 P=1 AVL=0 L=1 DB=0 G=1
```

At this point, looks like the IRET stack of Linux is wrong. Instead of using
CS=0x13 and SS=0xb, Linux should use CS=0x33 and SS=0x2b. Use QEMU to verify
this behavior.

To debug Linux, remove kaslr and aslr. For ASLR, create a file
`/etc/sysctl.d/01-disable-aslr.conf` with `kernel.randomize_va_space = 0`. Then
after reboot, `/proc/sys/kernel/randomize_va_space` should be `0`.
Ref: <https://askubuntu.com/a/318476>.

While reading Linux source `arch/x86/entry/entry_64.S` function
`entry_SYSCALL_64`, looks like a failure in syscall return can result in
executing IRET with incorrect CS. In `entry_SYSCALL_64`, there are a lot of
calls to `swapgs_restore_regs_and_return_to_usermode`. The guessed call stack
that triggers this bug is
```
entry_SYSCALL_64
	swapgs_restore_regs_and_return_to_usermode
		native_iret (INTERRUPT_RETURN, see arch/x86/include/asm/irqflags.h)
			native_irq_return_iret
				IRETQ
```

Current ideas
* Use monitor trap to print Linux control flow (disable interrupts)
	* Also try debug registers (e.g. DR7)
* Review SYSCALL related VMCS and MSRs
* Try not protecting `MSR_K6_STAR`
* Try protecting `MSR_K6_LSTAR` etc

In `xmhf64 77921d016..6e56fffcf`, I removed protection of `MSR_K6_STAR` in
XMHF. Then looks like the error disappears mysteriously. However, this should
be investigated as the remaining MSRs like EFER may still suffer from this bug.

The root cause of this problem is that `control_MSR_Bitmaps_address` is set by
Virtual Box, but XMHF does not recognize this field. For now, should remove
support for MSR bitmap. Can implement support in the future. MSR bitmap removed
in `xmhf64-nest a1215598b..e58ae3fb2`.

Also, through printing, looks like most of the MSR bitmap of VirtualBox are 1.
So I am guessing that the MSR bitmap may not be too helpful?

### Review XMHF managed MSRs

KVM performs a similar job of `vmx_msr_area_msrs` in function `setup_msrs()`.
This is Linux version v5.10.83. In newer Linux version (e.g. v5.19.11), the
function name becomes `vmx_setup_uret_msrs()`.

KVM manages the following MSRs

|Linux name         |Intel name     |XMHF Need|Comment                       |
|-------------------|---------------|---------|------------------------------|
|`MSR_STAR`         |`IA32_STAR`    |no       |XMHF does not handle syscalls |
|`MSR_LSTAR`        |`IA32_LSTAR`   |no       |XMHF does not handle syscalls |
|`MSR_SYSCALL_MASK` |`IA32_FMASK`   |no       |XMHF does not handle syscalls |
|`MSR_EFER`         |`IA32_EFER`    |yes      |XMHF may enter x64 mode       |
|`MSR_TSC_AUX`      |`IA32_TSC_AUX` |no       |XMHF does not use RDTSCP      |
|`MSR_IA32_TSX_CTRL`|`IA32_TSX_CTRL`|no       |XMHF does not use transactions|

So there are no needs to add to `vmx_msr_area_msrs`. As for `MSR_IA32_PAT`,
not sure why Linux does not use it. However, XMHF should have it.

### VMware

VMware Workstation Player can be downloaded for free at
<https://www.vmware.com/products/workstation-player.html>. I downloaded version
16.2.4 (Release Date 2022-07-21).

### VMware exiting immediately

With XMHF vmware, when starting a hypervisor, vmware exits immediately without
error messages. Without XMHF, vmware works fine. In `vmware/*/vmware.log`, see
the possible cause.
```
In(05)+ vmx VMware Player does not support the user level monitor on this host.
In(05)+ vmx Module 'MonitorMode' power on failed.
In(05)+ vmx Failed to start the virtual machine.
In(05)+ vmx 
```

However, it is strange because XMHF supports EPT. Maybe vmware is referring to
some other feature related to EPT. By not clearing some bits in
`xmhf_nested_arch_x86vmx_vcpu_init()`, we can bisect the features vmware wants

* Result: at least executes VMXON
	* Keep `INDEX_IA32_VMX_BASIC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_ENTRY_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_MISC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS2_MSR`
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: exit immediately
	* Change `INDEX_IA32_VMX_BASIC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS_MSR`
	* Change `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* Change `INDEX_IA32_VMX_ENTRY_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_MISC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS2_MSR`
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: at least executes VMXON
	* Change `INDEX_IA32_VMX_BASIC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_ENTRY_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_MISC_MSR`
	* Keep `INDEX_IA32_VMX_PROCBASED_CTLS2_MSR`
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: at least executes VMXON
	* Change `INDEX_IA32_VMX_BASIC_MSR`
	* Change `INDEX_IA32_VMX_PROCBASED_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_ENTRY_CTLS_MSR`
	* Change `INDEX_IA32_VMX_MISC_MSR`
	* Change `INDEX_IA32_VMX_PROCBASED_CTLS2_MSR`
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: exit immediately
	* Keep `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* Keep `INDEX_IA32_VMX_ENTRY_CTLS_MSR`
	* Change `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: exit immediately
	* Keep `INDEX_IA32_VMX_EXIT_CTLS_MSR`
	* `INDEX_IA32_VMX_ENTRY_CTLS_MSR`: change `CET_STATE`, keep others
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: at least executes VMXON
	* `INDEX_IA32_VMX_EXIT_CTLS_MSR`: keep EFER and PAT, change CET
	* `INDEX_IA32_VMX_ENTRY_CTLS_MSR`: keep EFER and PAT, change CET
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`
* Result: at least executes VMXON
	* `INDEX_IA32_VMX_EXIT_CTLS_MSR`: keep EFER and PAT, change CET
	* `INDEX_IA32_VMX_ENTRY_CTLS_MSR`: keep EFER and PAT, change CET
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`, keep 2M page and 1G page, change
	  "accessed and dirty flags"
* Result: at least executes VMXON
	* `INDEX_IA32_VMX_EXIT_CTLS_MSR`: keep EFER and PAT, change CET
	* `INDEX_IA32_VMX_ENTRY_CTLS_MSR`: keep EFER and PAT, change CET
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`, keep 2M page, change 1G page and
	  "accessed and dirty flags"
* Result: exit immediately
	* `INDEX_IA32_VMX_EXIT_CTLS_MSR`: keep EFER and PAT, change CET
	* `INDEX_IA32_VMX_ENTRY_CTLS_MSR`: keep EFER and PAT, change CET
	* Keep `INDEX_IA32_VMX_EPT_VPID_CAP_MSR`, change 2M page, 1G page, and
	  "accessed and dirty flags"

From the above experiments, we need to support 2 extra features
* Load `IA32_PAT` and `IA32_EFER`
* EPT 2M and 1G page

Also the "at least executes VMXON" result is actually vmware constantly running
the following instructions: VMXON, VMPTRLD, INVVPID, VMCLEAR, VMXOFF. 2 things
are suspective:
* Why does vmware execute VMXOFF? probably INVVPID failed?
* Why does vmware perform VMPTRLD without VMCLEAR? vmware bug?

The VMXOFF problem is not very stable. Should first implement the features.

TODO: implement features
TODO: check whether INVVPID fails / why VMXOFF
TODO: VMware exits immediately

