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

TODO: <https://serverfault.com/questions/198391/>

