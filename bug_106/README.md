# Trustvisor in L2

## Scope
* Nested virtualization, all configurations
* `lhv 83723a519`
* `xmhf64 28ce646f2`
* `xmhf64-nest 9ba342c98`
* `xmhf64-nest-dev 186e6588f`

## Behavior
The current design does not support running TrustVisor in L2, because all
VMCALLs from L2 are forwarded to L1. However, when Hyper-V is enabled, Windows
becomes an L2 guest.

Challenges include:
* Find a calling convention for TrustVisor, without comflict with L2 to L1
  interface
* L2 may request a page in EPT12, but is not yet in EPT02
* Implement "remove a page from EPT02", need to also remove the page in EPT01
* Check to make sure TrustVisor does not access VMCS using `vmcs->vcpu`
	* Need to extend functions such as `VCPU_grip()`
* L2 cannot produce any 201 VMEXIT during PAL

## Debugging

### Passing arguments to TrustVisor

We probably need to pass a vmcall offset parameter to TrustVisor. Currently
there are existing parameters:
* `hypapps/trustvisor/Makefile`: `export LDN_TV_INTEGRATION := n`
	* Hardcoded argument in Makefile
* `hypapps/trustvisor/configure.ac`: `--with-libbaremetalsrc`
	* However, in Makefile, `./configure` is called without arguments. The
	  configured value depends on environment variables

Maybe we should configure in XMHF directly (add "TrustVisor" to config name),
then pass the value to TrustVisor.

### XMHF and Hypapp interface

It is important to remember that XMHF uses a event based interface, as
mentioned in XMHF paper.

The following are existing interfaces in XMHF64:

* Interface for XMHF to forward event to Hypapp: see `xmhf-app.h`
	* `xmhf_app_main()`: initialize hypapp, no quiesce
	* `xmhf_app_handleintercept_portaccess()`: guest accesses invalid IO port,
	  quiesce / no quiesce
	* `xmhf_app_handleintercept_hwpgtblviolation()`: guest accesses invalid EPT,
	  quiesce
		* Arguments: gpa, gva, violationcode (rwx)
	* `xmhf_app_handleshutdown()`: guuest restarts, no quiesce
	* `xmhf_app_handlehypercall()`: guest performs VMCALL, quiesce
	* `xmhf_app_handlemtrr()`: guest changes MTRR, quiesce
	* `xmhf_app_handlecpuid()`: guest accesses CPUID, no quiesce
* Interface for Hypapp to access data in XMHF
	* Access vcpu from event call arguments: `VCPU *vcpu`
	* Access registers from event call arguments: `struct regs *r`
	* Access EPTP from vcpu: `xmhf_memprot_arch_x86vmx_get_EPTP` /
	  `xmhf_memprot_arch_x86vmx_set_EPTP`:
	* Access guest VMCS fields from vcpu (`xmhf-baseplatform-arch-x86.h`)
		* Functions: `VCPU_gcr3`, `VCPU_gcr4`, `VCPU_glm`, `VCPU_g64`, ...,
		  `VCPU_reg_get`, `VCPU_reg_set`
		* Accesses: GDT, RFLAGS, RIP, RSP, CR0, CR3, CR4, long mode, PDPTE,
		  wrapper for registers
		* Note: `VCPU_glm` accesses `control_VM_entry_controls` (control field)
		  instead, but for this bit VMCS02 = VMCS12
	* Walk page table from EPTP / CR3 using custom logic (HPT library)
	* Modify page table entries from EPTP / CR3 (e.g. `hptw_set_prot()`)
		* See `scode_lend_section()`, `scode_return_section()`
	* Modify DMA page table entries: `xmhf_dmaprot_protect()` /
	  `xmhf_dmaprot_unprotect()`
	* Flush TLB: `xmhf_memprot_flushmappings_alltlb()`
	* Flush DMA TLB: `xmhf_dmaprot_invalidate_cache()`
* Interface defined by TrustVisor for the guest to call TrustVisor: see
  `trustvisor.h`
	* VMCALL with EAX in `enum HCcmd`

Possible vulnerability about TrustVisor found:
* I think `tv_app_handleshutdown()` should check whether there are PALs still
  running. If so, should clear the memory of them. Otherwise, can reboot to a
  malicious OS and dump memory.
* Note that if the hypervisor crashes (e.g. due to the INIT-SIPI-SIPI exploit),
  `tv_app_handleshutdown()` will not be called and PALs become vulnerable.
  However this is XMHF's fault, not TrustVisor

Planned change to interfaces
* Interface for XMHF to forward event to Hypapp
	* Existing events handle both VMEXIT10 and VMEXIT20
		* For VMCALL, define a specific range of EAX that should be handled by
		  hypapp instead of L1
		* For EPT, perform EPT01 and EPT12 walk as usual. If EPT12 fails, let
		  L1 handle. If EPT01 fails, let hypapp handle.
	* Call when EPTP02 changes (no quiesce, event triggered by flush of EPT02)
	* Call when VMEXIT201 (no quiesce)
	* Call when VMEXIT102 (no quiesce, because symmetrical to VMEXIT201)
	* Maybe: call when VMEXIT202
* Hypapp to access data in XMHF
	* Add `VCPU_gnest` to indicate whether in nested virtualization
	* Functions in `xmhf-baseplatform-arch-x86.h` (e.g. `VCPU_gcr3`): if in L2,
	  use VMCS02 instead of VMCS01.
	* EPTP functions: need to access both EPTP01 and EPTP02. Normally hypapps
	  want to access EPTP02, but TrustVisor also accesses EPTP01 to make EPTP01
	  the same across all CPUs
		* `xmhf_memprot_arch_x86vmx_get_EPTP` /
		  `xmhf_memprot_arch_x86vmx_set_EPTP`: use EPT01 or EPT02 depend on VMX
		  operation mode
		* `xmhf_memprot_arch_x86vmx_get_EPTP01` /
		  `xmhf_memprot_arch_x86vmx_set_EPTP01`: always use EPT01
		* If TrustVisor changes EPTP01 (at this time other CPUs are quiesced),
		  normally requesting TLB flush should be able to sync the change to
		  other CPUs.
	* Walk EPT02: require new function because
		* EPT02 is generated dynamically from EPT01 and EPT12
	* Modify page table entries from EPTP: require new function because
		* Hypapp specifies address based on EPT02, XMHF needs to update EPT01
		* In the future, can try merging small pages in EPT to form bigger pages
	* Modify DMA page table entries: currently do not change because DMAP page
	  address focuses on the "0" in EPT01 / EPT02
	* Flush TLB / DMA TLB: remain the same (invalidates all EPT02s on all CPU)
* Interface defined by TrustVisor for the guest to call TrustVisor: see
  `trustvisor.h`
	* VMCALL has an offset added to distinguish from interface between L2 and L1
		* Currently using 0x4c415000U "?PAL"
		* TrustVisor will check during "Call when VMEXIT201". If match, it
		  becomes VMEXIT202
* Extra features
	* For each XMHF generated event, use a bool to allow hypapp to control
	  quiesce or not?
	* Initiate quiesce in hypapp code? We cannot quiesce everytime there is a
	  VMEXIT20 due to EPT / VMCALL, but if the event is related to TrustVisor
	  we need to quiesce.

Addressed tasks:
* Document existing XMHF interfaces
* How to pass configuration to TrustVisor? need to defined vmcall offset

Other hypapps
* `helloworld`: trivial
* `lockdown`: only other hypapp in XMHF's repository
* `quiesce`: looks like test case, ignored
* `trustvisor`: analyzed above
* `verify`: looks like test case, ignored

Looks like lockdown is more complicated in these ways:
* Which system needs nested virtualization? Only red? Or both?
* System switch happens due to user triggered event. What if at that time the
  system is in L2?

Currently we do not consider the compatibility of lockdown.

Addressed tasks:
* Consider reviewing other existing hypapps

TODO: test using LHV
TODO: review why `xmhf_app_handlecpuid()` is used, likely unnecessary

