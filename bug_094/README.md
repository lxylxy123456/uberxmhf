# Test other hypervisors (VMware)

## Scope
* All configurations, focus on amd64
* `xmhf64 710e9a847`
* `xmhf64-nest c44296487`
* `lhv c45283b0d`

## Behavior
Should try running VMware on XMHF.

## Debugging

Planned hypervisor to try on Linux
* Virtual Box
* VMware
* Xen

### Installing VMware

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

The VMXOFF problem is not very stable. Sometimes I see VMXON VMXOFF instead.
Should first implement the features. For 2M and 1G pages, looks like XMHF
already supported them in `bug_088` git `xmhf64-nest 6355bd66f..e1fe73dc8`. So
we can simply remove restrictions. Implemented in `xmhf64-nest 65a4c0e44`.

To implement load PAT / EFER, we need to review VMEXIT/VMENTRY checks and load
behaviors. CET is too complicated to be implemented for now.
* VMENTRY
	* Check (25.3.1.1)
		* `IA32_PAT`: each bit in 0, 1, 4, 5, 6, 7
		* `IA32_EFER`: reserved bits, LMA = "host address-space size",
		  LMA = LME if CR0.PG = 1
	* Load (25.3.2.1)
		* `IA32_PAT`: load directly
		* `IA32_EFER`: load directly
		* If not loading `IA32_EFER`, have different logic
* VMEXIT
	* Check (25.2.2)
		* `IA32_PAT`: each bit in 0, 1, 4, 5, 6, 7
		* `IA32_EFER`: reserved bits, LMA = LME = "host address-space size"
	* Save (26.3.1)
		* `IA32_PAT`: save directly
		* `IA32_EFER`: save directly
	* Load (26.5.1)
		* `IA32_PAT`: load directly
		* `IA32_EFER`: load directly
		* If not loading `IA32_EFER`, have some logic. If loading, the effect
		  of the logic seems to be overwritten

This feature is implemented in `xmhf64-nest 9d62746cb`. Main jobs done are:
* Remove enable bits in vmexit / vmentry controls for VMCS02
* In VMCS02 and VMCS12 translation functions, copy VMCS fields to MSR
  load / save areas
* Implement `_check_ia32_pat`, `_check_ia32_efer`
* Update part about where to update etc

Looks like VirtualBox can be used to verify whether the implementation is
correct, because VirtualBox uses this feature if available.

Then in `xmhf64-nest 9d62746cb..0b560ab7d`, perform some clean up
* Reduce number of memcpys in nexted-x86vmx-vmcs12.c
* Update MSR variable names

Then see an XMHF assertion failure when vmware is trying VMENTRY. It turns out
that vmware is setting I/O bitmaps to 0xffffffffffffffff, but XMHF always
expects it to be a valid address. Fixed in `xmhf64-nest 63ef633f5`.
```
vmcs12->control_VMX_cpu_based = 0xa5a07dfa
vmcs12->control_IO_BitmapA_address = 0xffffffffffffffff
vmcs12->control_IO_BitmapB_address = 0xffffffffffffffff
```

Looks like vmware will perform MOV CR4 and VMXON very frequently (probably for
each timer interrupt?), so we need to optimize MOV CR4 operations. In old XMHF
code, the CR4 intercept handler performs INVVPID unconditionally.
```
440  	#if defined (__NESTED_PAGING__)
441  	//we need to flush EPT mappings as we emulated CR4 load above
442  	__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, 1, 0);
443  	#endif
```

However, it looks like not necessary. So in `xmhf64 710e9a847..427b6d6b7`, I
removed this INVVPID call.

Also, vmware seems to set `control_Executive_VMCS_pointer` to non-zero value
(probably 0xffffffffffffffff, but I did not check). Looks like this field is
only relevant when XMHF is in SMM. So change XMHF to ignore this field in
`xmhf64-nest 72533f6aa..0d7e80ad1`. See also this comment
<https://bugzilla.kernel.org/show_bug.cgi?id=216091#c1>.

After that, vmware can boot into debian11x64 (QEMU image converted to vmdk).

### Creating multiple VMware virtual machines

Now it is time to test more L2 guest systems on VMware / Virtual Box. First,
VMware workstation player does not support disk formats other than VMDK. So we
need to convert QCOW2 format to VMDK.
```
qemu-img convert $i.qcow2 -O vmdk $i.vmdk
```

Thanks to <https://serverfault.com/questions/198391/>, looks like we just need
to manually create a `.vmx` file in order to create a virtual machine for
VMware. Use `vmware.py` in this bug folder. Then use bash script
```sh
for i in debian11x64 debian11x86 ...; do
	mkdir $i && python3 vmware.py $i .../$i.vmdk > $i/$i.vmx
done
```

When opening the virtual machine, select "I Copied it"

VMware test result
* debian11x64: good
* debian11x86-p: good
* debian11x86: good
* win10x64 bluescreen, code `INACCESSIBLE_BOOT_DEVICE`
* win10x86: bluescreen, code `INACCESSIBLE_BOOT_DEVICE`
* win7x64
* win7x86: bluescreen
* winxpx64: bluescreen
* winxpx86: bluescreen

TODO: check whether need to select correct OS type

### Creating multiple VirtualBox virtual machines

For virtual box, use `VBoxManage`
VBoxManage modifyvm winxpx64 --hda /media/dev/Last/winxpx64.vmdk
```
VBoxManage createvm --name $i --register
VBoxManage storagectl $i \
                      --name "SATA Controller" \
                      --add sata \
                      --controller IntelAHCI \
                      --portcount 1 \
                      --bootable on
VBoxManage storageattach $i \
                         --storagectl "SATA Controller" \
                         --device 0 \
                         --port 0 \
                         --type hdd \
                         --medium $disk_file
```

* Ref: <https://serverfault.com/questions/171665/>

I tried winxpx64. However, I realized that for VirtualBox and VMware, opening
a Windows that was installed from KVM will result in blue screen, even not
running nested virtualization. So we need to install Windows from scratch.

### VMXON twice problem

When running vmware VM, if start a virtual box VM, the CPU will execute VMXON
twice. Currently this is not implemented by XMHF, and XMHF prints
"Not implemented, HALT!". See `xmhf_nested_arch_x86vmx_handle_vmxon()`.

Fixing in `xmhf64-nest 0bd29bc66`. However, the problem now becomes an
assertion error. This is because VMPTRLD is called with no active VMCS pointer.
Fixed in `xmhf64-nest 3ca7e20f5` by checking for this special case.

### Testing Windows as guest

At `xmhf64-nest-dev c91401f26` (`xmhf64-nest 4bfe710ab`), using
`./build.sh amd64 --dmap --mem 0x230000000 --event-logger` to build XMHF.

After some time, realized that I should use O3 instead. Also currently Dell
does not support DRT (`bug_097`), HP does not support DMAP (`bug_098`). So
`./build.sh amd64 O3 --mem 0x230000000 --event-logger`.

* HP 2540p, amd64 XMHF, Virtual Box, Win7 x86: good (install and run)
* Dell 7050, amd64 XMHF, VMware, WinXP x86: good (install and run)
* Dell 7050, amd64 XMHF, VMware, WinXP x64: good (install and run)
* Dell 7050, amd64 XMHF, VMware, Win10 x64: good (install and run)
* HP 2540p, amd64 XMHF, Virtual Box, Win7 x64: good (install and run)
* HP 2540p, amd64 XMHF, Virtual Box, Win10 x86: unknown (too slow to install)
* Dell 7050, amd64 XMHF, Virtual Box, Win10 x86: good (run)
* Dell 7050, amd64 XMHF, Virtual Box, Win10 x64: good (run)
* Dell 7050, amd64 XMHF, KVM UP, Win10 x86: good (run)
* Dell 7050, amd64 XMHF, KVM UP, Win10 x64: good (run)
* Dell 7050, amd64 XMHF, KVM SMP, Win10 x86: good (run)
* Dell 7050, amd64 XMHF, KVM SMP, Win10 x64: good (run)

TODO: test XMHF, Windows, VB/VMW
TODO: use circleci to speed up nested virtualization

During testing, I noticed that there are a lot of `ept02_miss` events. It is
probably because L1 is executing the INVEPT instruction. Currently XMHF simply
drops the entire EPT02. To improve efficiency, we may need to mark entries in
EPT12 as read only and only invalidate part of EPT02. We need to study shadow
paging. Sample references:
* <https://www.kernel.org/doc/Documentation/virtual/kvm/mmu.txt>
* <https://stackoverflow.com/questions/14176904/>

### Testing Windows as L1 host

We also test running XMHF, Windows 10 x64, Nested guest. Planned hypervisors
are VirtualBox, VMware, Hyper-V (if supported). Then will try virtualization
based security.

* HP, amd64 XMHF, win10x64, VirtualBox, win10x86: see mouse and black screen
  (too slow to reach login screen)
* HP, amd64 XMHF, win10x64, VirtualBox, win10x64: see mouse and black screen

## Fix

`xmhf64 710e9a847..427b6d6b7`
* Remove unneeded INVEPT in MOV CR4

`xmhf64 153a61f99..8b972c159`
* Remove unneeded INVEPT in MOV CR0

`xmhf64-nest c44296487..3ca7e20f5`
* Indicate support for 2M and 1G EPT pages for L1
* Support load / store `IA32_PAT` / `IA32_EFER`
* Do not resolve I/O bitmap address when not using
* Ignore value of `control_Executive_VMCS_pointer`
* Correctly handle VMXON twice problem

