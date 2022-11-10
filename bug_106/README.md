# Trustvisor in L2

## Scope
* Nested virtualization, all configurations
* `lhv 83723a519`
* `lhv-dev a41a04b0c`
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

Other hypapps shipped with XMHF code:
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

Discussed with superymk. `xmhf_app_handlecpuid()` is needed because guest needs
a way to detect presence of XMHF / Hypapp. However, I changed the call
interface a little bit in `xmhf64 19978269c` to make it usable in L2.

Addressed tasks:
* Review why `xmhf_app_handlecpuid()` is used, likely unnecessary

In `lhv e19f3c178`, revise LHV to be able to preserve kernel mode (ring 0)
state while running in user mode. In `lhv-dev 742b17717`, able to call
TrustVisor from LHV in L2. This makes testing and debugging with QEMU easy.

Addressed tasks:
* Test using LHV

### TrustVisor handling of EPT

We review how TrustVisor uses EPT, because there are a lot of changes.

* Access to EPT is handled in `hpt_emhf.h` using functions
	* `hpt_emhf_get_root_pm_pa`: get EPTP physical address
	* `hpt_emhf_get_root_pm`: get EPTP virtual address
		* Used in `hptw_emhf_host_ctx_init_of_vcpu()`: see below
	* `hpt_emhf_set_root_pm_pa`: set EPTP physical address
		* Used in `hpt_scode_switch_scode()` to change EPT02
	* `hpt_emhf_set_root_pm`: set EPTP virtual address
		* Used when first time calling `scode_register()`, to switch EPT01
		* Used in `hpt_scode_switch_regular()` to change EPT02
	* `hpt_emhf_get_root_pmo`: get EPTP virtual address and some metadata
		* Used when first time calling `scode_register()`, `g_reg_npmo_root` is
		  considered the common EPT across all CPUs
	* `hptw_emhf_host_ctx_init_of_vcpu()`: construct class to walk EPT02
		* Used when first time calling `scode_register()`, `g_hptw_reg_host_ctx`
		  is used to walk EPT02 (including walk guest page table)
		* Used by `hptw_emhf_checked_guest_ctx_init_of_vcpu()`, similarly is
		  used to construct class to walk gust page table.

For TrustVisor, the functions provided by OS need to be compatible with the
hptw library. The requirements are reflected in `hptw_ctx_t`.
* gzp: not supported
* pa2ptr: need to walk EPT02, two choices
	1. Walk the computed EPT02, when failure call
	   `xmhf_nested_arch_x86vmx_handle_ept02_exit()` and retry
	2. 2D walk EPT12 and EPT01
* ptr2pa: not supported
* Also need to keep track of `ept02_cache_line_t * cache_line`

For removing memory, can simply change EPT01. This is easy because EPT01 is
unique.

#### About searching for PAL

Currently `scode_in_list()` is used to search for PALs for a match while
entering scode. The following need to match
* `gcr3`: guest CR3
	* Assumption: guest CR3 does not change, even across different CPUs
* `g64`: whether guest is in 64-bit mode
* `gvaddr`: e.g. guest RIP
* Assumption: EPT01 never changes.

Examples of breaking the CR3 assumption (not likely in a normal OS):
* Different CR3s are created for the same physical memory (same CPU), e.g.
  `mmap()` with `MAP_SHARED` then `fork()`
* For different CPUs, different CR3s are used (for the same process)

After supporting nested virtualization, we also need to distinguish different
L2 address spaces. There are 2 possible solutions
* Distinguish using EPT02: does not work with current shadow EPT implementation
* Distinguish using EPT12: just need to assume that L1 hypervisor uses the same
  EPT across different CPUs. This is a reasonable assumption for Xen / Hyper-V
  architecture
	* Note that XMHF in L1 breaks this assumption

Another solution is to radically change how PALs are implemented. A whitelist
specifies all pages (L0 physical address) and virtual addresses. It does not
care number of levels of translations in between. TrustVisor will build a new
CR3 from scratch. For example whitelist spec looks like:
* entry: va=0x80001234, pa=0x0aaa1234
* .text: va=0x80001000, pa=0x0aaa1000
* .data: va=0xcccc1000, pa=0x0aaa3000
* param: va=0xcccc2000, pa=0x0aaa5000
* stack: va=0xffff1000, pa=0x0aaa7000

#### TrustVisor Vulnerability

Found a possible vulnerability in TrustVisor's implementation. In `pt.c`
function `scode_lend_section()`, permission checking is done on
1. Guest CR3 satisfies `section->reg_prot`
2. XMHF EPT satisfies `section->pal_prot`

For example, for data section:
* `section->reg_prot` is 0
* `section->pal_prot` is RW

1 is wrong. Should also check `section->pal_prot`. This can cause memory
integrity of the guest OS to be broken. Example attack:
* Start process A (privileged), have some mmaped memory X
* Start process B, map the same memory X with read only
* Process B starts PAL, register memory X as data section
* PAL modifies content of memory X, will be able to modify
* Process A will read incorrect value

This vulnerability is fixed in `xmhf64-nest-dev 3708b0457`. Cherry-picked in
`xmhf64-nest 06faedbde`.

Future work: implement this exploit

TODO: compiler bug in xmhf64-nest-dev: `xmhf_nested_arch_x86vmx_get_ept12()`
TODO: `// TODO: replace with hptw_checked_get_pmeo()`
TODO: Make sure 3708b0457 works (does not break things)

TODO
TODO: `#### TrustVisor Vulnerability` above

