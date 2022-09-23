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

The relevant SDM chapter is Intel i3 "25.4 LOADING MSRS". This footnote looks
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

TODO: Check what it means to set guest ia32e=0. Will it clear efer.lme automatically? See Intel footnote
TODO: will it work if we simply copy VirtualBox's VMCS12 to VMCS02?
TODO: try running KVM VirtualBox and KVM XMHF VirtualBox

TODO: fix i386 vmentry failure
TODO: does LHV have the same problem?
TODO: fix amd64 kill init problem
TODO: review KVM's `setup_msrs()`, see whether XMHF's `vmx_msr_area_msrs` need change

