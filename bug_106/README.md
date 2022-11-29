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

#### TrustVisor Vulnerability 1

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

### Possible compiler problem

In `xmhf64-nest-dev aefee5870`, function
`_vmcs12_to_vmcs02_control_EPT_pointer()`, variable `ept12` is used before
initialized. A smaller example is:

```c
void lxy_vmcs12_to_vmcs02_control_EPT_pointer(ARG10 * arg)
{
	spa_t ept02;
	if (_vmx_hasctl_enable_ept(arg->ctls)) {
		u64 eptp12 = arg->vmcs12->control_EPT_pointer;
		gpa_t ept12;
		ept02_cache_line_t *l;
		bool h;
		arg->vmcs12_info->guest_ept_root = ept12;
		xmhf_nested_arch_x86vmx_check_ept_lower_bits(eptp12, &ept12);
		ept02 = xmhf_nested_arch_x86vmx_get_ept02(arg->vcpu, ept12, &h, &l);
	}
	__vmx_vmwrite64(VMCSENC_control_EPT_pointer, ept02);
}
```

GCC version is `gcc (GCC) 12.2.1 20220819 (Red Hat 12.2.1-2)`.

The bug caused problem when running LHV with EPT and XMHF. Fixed in
`xmhf64-nest-dev 099e81c46`. After fixing this bug, `--lhv-opt=0xdfd` can run
well under XMHF.

Also interestingly, Jenkins was able to catch this bug as a regression in the
`Jenkinsfile_nest` pipeline.

The GCC bug is minimized to independently reproducible code `a1.c`. Compile
command is `gcc -Wall -Werror -c a1.c`. Then minimized further to `a2.c`.

Bug report text in `bug.txt`, reported bug to
<https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107663>, tracked in `bug_076`.

### Testing hypervisors other than LHV

We test XMHF XMHF LHV (call nested virtualization). i.e. L2 = LHV host,
L3 = LHV guest. Since the guest running PAL must make sure the EPT entries for
PAL are present, we cannot enable all LHV opts.

Git `lhv-dev 047695f86`, `xmhf64-nest-dev 632b179a2`. On Thinkpad,
`./build.sh amd64 --lhv-opt 0xfbc` with smp=4 works fine.
* 0x200 (`LHV_NO_INTERRUPT`) is used to remove interrupts because handling
  interrupts would be too slow.
* <del>0x1 (`LHV_USE_MSR_LOAD`) is not used because L1 will modify and
  invalidate EPT (due to MTRR).</del>
	* Later, looks like 0x1 (`LHV_USE_MSR_LOAD`) can be added, because MTRR
	  does not cause flush of EPTs on other CPUs. However, MTRR is very slow.
* 0x40 (`LHV_USER_MODE`) is not used because L1 will modify and invalidate EPT
  (due to calling TrustVisor on L1).
	* When this option is added, see error message:
	  `hpt.c:804: assert failed: hpt_type_is_valid(t)`

However, when only smp=1 in KVM, `--lhv-opt 0xffc` works well.

Then we test the following configuration:
* Dell
	* XMHF amd64 (L0)
		* Debian 11 x64 (L1)
			* KVM
				* Debian 11 x64 (L2)
					* LHV

See this error message:
```
Fatal: Halting! Condition 'hpt_pmeo_get_address(&page_reg_npmeo01) == page_reg_spa' failed, line 286, file pt.c
```

From print debugging, there is
* `hpt_pmeo_get_address(&page_reg_npmeo01) = 0x0e4d1000`
* `page_reg_spa = 0x1830d1000`

Looks like there is a typo. When walking EPT01, the virtual address
`page_reg_gpa` (meaning L2) is used. But should use `page_reg_spa` (L0 = L1)
should be used. Fixed in `xmhf64-nest-dev ced04d09d`.

Now when running `./test_args32L2 1 1 1` on
`Dell -> XMHF -> Debian -> KVM -> Debian x64`, see:
```
TV[3]:scode.c:hpt_scode_npf:1378:                  EU_CHK( ((whitelist[*curr].entry_v == rip) && (whitelist[*curr].entry_p == gpaddr))) failed
TV[3]:scode.c:hpt_scode_npf:1378:                  Invalid entry point
```

For more debug info see serial `20221114142745`. Can see that something strange
is happening in XMHF / TrustVisor. During scode register it says
* `CR3 value is 0x5987801, entry point vaddr is 0xf7f7e025, paddr is 0xe4cf025`

But when entering scode,
* rip is correct: `rip = 0x00000000f7f7e025`
* gpaddr becomes another page: `gpaddr = 0x00000001574cf025`
* a totally different address is stored in whitelist info:
  `whitelist[*curr].entry_v = 0x000000002907a9c0`,
  `whitelist[*curr].entry_p = 0x000000002907a9c0`

Serial `20221115124920` contains more print debugging, see lines starting with
"LXY". Looks like print debugging code at line 1390 - 1400 is wrong. Should
print after the `*curr = index;` line.

Fixed above and serial `20221115125525`. Can see that the problem is that
`whitelist[?].entry_p` is wrong.
* 0x0bfa4025 is the result during page table walk
* 0xf7f3e025 is the gpa while XMHF calls EPT violation routine in TV

In `20221115131135`, also add some prints in nested virtualization. Can see
that `whitelist[*curr].entry_p` is incorrect. It stores L2 paddr, but should be
L1 paddr. Need to check `scode_register()`.

The problem is in use of `hptw_va_to_pa()` function. When walking the page
tables HPT will perform 3D walk (gCR3, EPT12, EPT01), but `hptw_va_to_pa()`
will only translate L2 virtual address to L2 physical address. TrustVisor need
to be modified to perform another translation to L1 physical address.

```
  whitelist_new.entry_v = gventry;
  whitelist_new.entry_p = hptw_va_to_pa( &reg_guest_walk_ctx.super, gventry);
```

Print debugging code is in `d3.diff`, not committed. Apply on
`xmhf64-nest-dev 6a1877def`. Now the problem is `tv_app_handle_nest_exit()`,
meaning that VMEXIT201 happens for unknown reason.

`l4.diff` is used to make TrustVisor log level verbose. It can be applied to a
lot of versions.

The VMEXIT reason is 1 (External interrupt). So we need to remove
"external-interrupt exiting" bit in "VM-execution control" during PAL. Fixed in
`xmhf64-nest-dev 5949650da`

Then see unstable VMEXIT reason 2 (triple fault). Looks like the problem
happens when unfortunate. Maybe the bug is reproduced within running PAL demo
10 times. The triple fault is because the exception bitmap is not set
correctly. Fixed in `xmhf64-nest-dev 92961ac06`.

The VMEXIT becomes reason 0 (exception), serial `20221115164630`. One thing to
notice is that in 2 tries, both of them have `guest_RIP % 0x1000 = 0x1bf`.
* `info_vmexit_interrupt_information = 0x80000b0e`
	* The exception is 0xe (page fault), 3: Hardware exception, valid error code
* `info_vmexit_interrupt_error_code = 0x00000004`
	* Bit 0 (P) = 0: page is not present
	* Bit 1 (R/W) = 0: falut due to read access
	* Bit 2 (U/S) = 1: access in user mode
	* Bit 3 (RSVD) = 0: CR3 etc does not set reserved bit
	* Bit 4 (I/D) = 0: fault not due to inst fetch
	* Bit 5 (PK) = 0: fault not due to protection keys
	* Bit 6 (SS) = 0: fault not due to shadow stack
	* Bit 7 (SGX) = 0: fault not due to SGX
* `info_exit_qualification = 0x00000000f7f86114`
	* Ref: Intel v3 says "A page fault does not update CR2. (The linear address
	  causing the page fault is saved in the exit-qualification field.)"
* `guest_GS_base = 0x00000000f7f86100`
	* Is the instruction accessing segment GS?

By debugging `./test_args32L2` as a process using GDB, see that the code page
is 0xf7fbc000, and offset 0x1bf is:`0xf7fbc1bf: mov    %gs:0x14,%eax`. So our
guess is correct. Actually this instruction is part of automatically added
stack canary:
```
00002675 <pal_5_ptr>:
    2675:	f3 0f 1e fb          	endbr32 
    2679:	55                   	push   %ebp
    267a:	89 e5                	mov    %esp,%ebp
    267c:	83 ec 78             	sub    $0x78,%esp
(handle arguments)
    269d:	65 a1 14 00 00 00    	mov    %gs:0x14,%eax
    26a3:	89 45 f4             	mov    %eax,-0xc(%ebp)
    26a6:	31 c0                	xor    %eax,%eax
(function body)
    2779:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    277c:	65 33 0d 14 00 00 00 	xor    %gs:0x14,%ecx
    2783:	74 05                	je     278a <pal_5_ptr+0x115>
    2785:	e8 fc ff ff ff       	call   2786 <pal_5_ptr+0x111>
    278a:	c9                   	leave  
    278b:	c3                   	ret    
```

It is strange why this only causes page fault sometimes.

Actually, with the newly compiled `pal_demo` from GitHub actions, running in L1
also produces the page faults due to access to GS segment. So `pal_demo` needs
to be modified.

Serial `20221116121251` is running TrustVisor in L2 with
`./test_args32L2 1 1 1` for 4 times. The last time fails. This actually reveals
another bug: `hptw_checked_get_pmeo(&page_reg_npmeo02, ...)` actually gets
EPT12 entry, not EPT02.

Serial `20221116122852` is running TrustVisor more times, and finally
triggering the page fault. Now I think I am able to answer: Why is the page
fault not deterministic? It is likely because I got confused while recording
experiment results. Actually `./test_args32L2 1 ...` does not generate page
fault. Only `./test_args32L2 4 ...` generates the page fault. By looking at
objdump results, only `pal_5_ptr()` among the pal functions accesses GS.

In `xmhf64-nest-dev 92961ac06..862b859dc`, fix the
`hptw_checked_get_pmeo(&page_reg_npmeo02, ...)` problem. The key problem is
that `page_reg_npmeo02` looks like EPT02 when accessing memory, but looks like
EPT12 when converting va to pa, or accessing page table entry. Looks like now
both bugs are fixed (GS and `hptw_checked_get_pmeo`). pal demo is still not
stable, though.

#### Assertion error `!(hpt_err)`

At `xmhf64-nest-dev 862b859dc`, testing
`Dell, XMHF, Debian, KVM, Debian, pal_demo`, sometimes see:
```
Fatal: Halting! Condition '!(hpt_err)' failed, line 263, file pt.c
```

This is the error of walking EPT12. This may be the fault of the guest
hypervisor, since KVM may invalidate EPT. Currently we ignore this error.

#### VMEXIT201 due to "VMX-preemption timer expired"

After enabling TrustVisor debug messages, running PAL becomes slow, and see
VMEXIT201 due to "VMX-preemption timer expired". Serial `20221123152543`.

By reading Intel v3, can see that there are mainly 3 related VMCS fields:
* "Pin-Based VM-Execution Controls" bit 6: "Activate VMX-preemption timer"
* "VM-Exit Controls" bit 22: "Save VMX-preemption timer value"
* "VMX-preemption timer value": 32-bit timer

The correct way to virtualize this is:
* When "Activate VMX-preemption timer" = 1, always
  "Save VMX-preemption timer value" = 1
* When VMEXIT202 begins, record RDTSC. When VMEXIT202 is about to end,
  perform RDTSC and re-calculate "VMX-preemption timer value" using the TSC
  difference.

This method is complicated, and is hard to be accurate. Currently we simply
ignore this field. It makes sense if we assume:
* Guest hypervisors (L1) do not use this timer for time tracking
* Guest hypervisors (L1) only uses this timer to make sure the CPU does not
  stuck in guest mode forever.

From KVM code `handle_preemption_timer()`, this assumption stands. Also
remember that most hypervisors use external interrupts to track time.

After making this assumption, the situation is:
* If guest "Save VMX-preemption timer value" = 1, then XMHF appears to be
  infinite speed.
* If guest "Save VMX-preemption timer value" = 0, then it expects upper bound
  time between VMENTRY and VMCS201. However XMHF sets upper bound time between
  VMCS202s. This will become a problem if L2 guest generates VMCS202's
  (e.g. EPT) forever.

For now, we keep the existing implementation. For TrustVisor, we need to add
interface to control VMX-preemption timer, similar to controlling interrupts.

Fixed in `xmhf64-nest-dev ff3716218`.

After fixing this, looks like "Assertion error `!(hpt_err)`" disappears. Now we
can basically assume XMHF KVM is stable.

Tested XMHF VirtualBox Debian x64, looks good.

Tested XMHF VMware Debian x64, looks good.

### Testing Windows as L2

At `xmhf64-nest-dev ff3716218`, found 2 problems:
* Normal configuration is `XMHF, VirtualBox, Windows 10 x86`. If add another
  qcow2 disk (just a FAT partition, no partition table), see Virtual Box Guru
  Meditation 1155 (triple fault) during early boot process
* Run `XMHF, VirtualBox, Windows 10 x86, pal_demo`, see VMENTRY02 error when
  starting pal demo.

Added VMCS dumping code, git apply `v5.diff`.

In serial `20221123201216`, tried to run `pal_demo` but dump all VMCS. Now the
problem becomes Virtual Box Guru Meditation 1155. From the VMCS dump can see
that PDPTEs are wrong. The problem is likely caused by TrustVisor code.
```
CPU(0x04): (0x280a) :VMCALL:guest_PDPTE0 = 0x0000000026ee9801
CPU(0x04): (0x280c) :VMCALL:guest_PDPTE1 = 0x0000000029aea801
CPU(0x04): (0x280e) :VMCALL:guest_PDPTE2 = 0x0000000000900801
CPU(0x04): (0x2810) :VMCALL:guest_PDPTE3 = 0x000000002fe34801
CPU(0x04): (0x280a) :3FAULT:guest_PDPTE0 = 0x6c62aadf26143ef2
CPU(0x04): (0x280c) :3FAULT:guest_PDPTE1 = 0x7466ba5736342ef2
CPU(0x04): (0x280e) :3FAULT:guest_PDPTE2 = 0xfc463a77b614aed2
CPU(0x04): (0x2810) :3FAULT:guest_PDPTE3 = 0xfc063a37b654ae92
```

Why does not the VMENTRY fail? Likely it depends on the content of PDPTE. See
Intel v3:
> A VM entry that checks the validity of the PDPTEs uses the same checks that
> are used when CR3 is loaded with
> MOV to CR3 when PAE paging is in use.4 If MOV to CR3 would cause a
> general-protection exception due to the
> PDPTEs that would be loaded (e.g., because a reserved bit is set), the VM entry fails.

The pal demo bug is fixed in the following 2 commits:
* `xmhf64 ae4c24db5`: during unmarshall, use the correct context
* `xmhf64-nest-dev 024be22d1`: do not use `ctx->pa2ptr` (unstable behavior with
  nested EPT); use `hptw_checked_copy_from_va`.

After fixing the pal demo bug, the "booting Windows 10 with extra disk and see
Meditation 1155" bug can no longer be reproduced. So not going to worry about
it for now.

Tested XMHF VirtualBox Windows 10 PAE, looks good.

### Testing Hyper-V

When testing on Hyper-V, the results is not stable
* (serial not recorded): TV "incorrect regular code EPT configuration!"
* Serial `20221124000644`: CPU 3 (APIC 6) halts for unknow reason, blue screen
* Serial `20221124115219`: similar to above, blue screen error code is
  `SYSTEM_SERVICE_EXCEPTION`
* Serial `20221124123051`: see EPT violation on code page due to 

For BSOD, the CPUs halt probably because Windows disabled interrupts. More
debugging shows that during BSOD, at first all APs halt. Then BSP also halts.
Now we need to find out why BSOD happens.

My guess for BSOD is that some VMEXIT201 causes the problem. However, it is
hard to see which one.

My guess for EPT violation is that some kind of antivirus program is scanning
the memory. Notice EPT violation CR3 and scode register CR3 are not the same.
Well, it is also possible that EPT calculation is wrong (e.g. due to 32-bit and
64-bit integer) and the wrong page is removed from EPT01.

Another observation is that I only see "scode register" and "scode unregister",
but there is no call to scode (e.g. scode marshall) in between. Is there a
problem with the EXE?
* Example: run `.\test_args32L2.exe 7 1 1`, see only 3 pairs of reg/unreg.

Serial `20221124132907`. Ran `.\test_args32L2.exe 7 1 1` 4 times and
`.\test_args32L2.exe 7 7 7` 2 times, all good. Then run
`.\test_args32L2.exe 7 70 7`, see EPT write violation. This time is CR3 the
same as scode register.

Serial `20221124141818`. Ran `.\test_args32L2.exe 7 7 7` 3 times and see the
error during the third run. This time is CR3 different from scode register. Can
also see that the RIP 0x7ff8d7ccf8dd is obviously not 32-bit. This means the
antivirus theory is more likely.

When Hyper-V is disabled, running XMHF Windows looks fine
* Running pal demo for a long time does not cause problems
* Scode is really called because see `scode_marshall()` is called

Confirmed that when running XMHF Debian QEMU Debian PAL, `scode_marshall()` is
called as expected.

When running on Windows (no Hyper-V), modify pal demo to sleep infinitely
instead of calling `scode_unregister()`. Then after a long time, see EPT
violation. Serial `20221124150731`.

The current guess is:
* Windows has antivirus that scans the memory, so this needs to be addressed
  in the future.
* However, the scan is not that frequent. So maybe the problem with Hyper-V
  happens as pal demo crashes earlier and postmortem program is scanning
  something.

Ideas
* Monitor L1 use of invept, also L0 (related to why scode not entered)
* Print process CR3 entries in hyper-v, also EPT (related to why invalid access)
* Add code to pal demo and trustvisor to detect scode presence
* For debugging, do not remove the page from regular EPT. Just add to pal EPT
* <del>Test pal demo, add long sleep time</del> (addressed: Windows 10 will
  break TrustVisor assumption)
* Randomly disable Windows security features (related to antivirus theory)
* Dump bluescreen information using BlueScreenView (3rd party freeware)
* Dump process memory map using VMMap (provided by Microsoft software)
* <del>Test Windows without hyper-v</del> (tested, mostly good)

At this point, debugging changes are in `l6.diff`. Switching to `xmhf64` branch
to add TrustVisor detection via CPUID.

### Update PAL demo

In `xmhf64 5331f23e2..d424154e1` and `xmhf64-nest-dev a7426131c..17fbc5260`,
update TrustVisor to respond to CPUID. When EAX=0x7a567254 (TrVz), and (if not
nested) ECX=0 or (if nested) ECX=vmcall offset, will activate
* EAX=0x7a767274 (TRVZ)
* EBX=`UINT_MAX` if not in PAL, or `whitelist_entry.id` if in PAL.
* ECX[0]=1 if 64-bit guest
* ECX[1]=1 if EPT12 in use.

The above commits also updated PAL demo to use CPUID. In `lhv-dev 88180d177`,
also update LHV to test CPUID.

The CPUID turns out to be effective.
* Dell, XMHF, Debian, KVM (smp=4), Debian, `./test_args32L2 7 700 7`: after 858
  tests, see PAL demo return 0xdead0002 (should be in scode, but not), and
  after some time see XMHF assertion error with
  `Fatal: Halting! Condition '!(hpt_err)' failed, line 263, file pt.c`
* Thinkpad, Fedora, KVM (smp=4), XMHF, LHV (opt=0x840): PAL demo returns
  incorrect result, likely similar to above.
* Thinkpad, Fedora, KVM (smp=3), XMHF, LHV (opt=0x840): confirmed that
  0xdeadbee2 is returned.
* Thinkpad, Fedora, KVM (smp=3), XMHF, LHV (opt=0x840): sometimes also see
  `Condition '__vmx_vmread64(VMCSENC_control_EPT_pointer) == ept02' failed`
  in vmcs12.c at CPU 0. Using GDB see `ept = 0x1021bda8` or similar. Strange
  because EPTP should not end with 0xda8.

The meaning of the above discoveries: these are likely problems encountered by
Hyper-V. We can now work on open source software. Also, we can work on
lightweight software. Also, we can work on GDB.

Modifying LHV arguments:
* `lhv-opt 0x40`, KVM smp=4: can see `ept = 0x1021bda8` bug
* `lhv-opt 0x240`, KVM smp=4: cannot reproduce the bug

Then after some testing, in KVM smp=4, see both CPU 1 and CPU 2 halt on the EPT
check (`val = __vmx_vmread64(VMCSENC_control_EPT_pointer)`:
```
(gdb) p/x ept02
$7 = 0x1426901e
(gdb) p/x val
$8 = 0x1426801e
(gdb) t 2
[Switching to thread 2 (Thread 1.2)]
#0  0x000000001021df86 in _vmcs02_to_vmcs12_control_EPT_pointer (arg=0x28f90d98 <g_cpustacks+130456>) at arch/x86/vmx/nested-x86vmx-vmcs12.c:975
975		HALT_ON_ERRORCOND(val == ept02);
(gdb) p/x ept02
$9 = 0x1426901e
(gdb) p/x val
$10 = 0x1426701e
(gdb) 
```

The bug is clear now. In `scode.c`, (see `g_did_change_root_mappings`) EPTs are
changed during the first time PAL is called. This mapping change will only
apply to `vcpu[id].vmcs.control_EPT_pointer` (i.e. VMCS01), but will not apply
to VMCS02. So if the quiesced CPU is in L2, it will trigger the assertion
error.

This is actually a regression caused by `xmhf64-nest-dev 582c2c1fb`.
Interestingly, I added a note in the commit saying TrustVisor may break it, but
then I forgot. Fixed in `xmhf64-nest-dev 6f2c0c862`.

### PAL not executed bug

Now we investigate why TrustVisor is not executed.
* When LHV opt=0x40, KVM smp=8, this problem is reproducible. So running
  TrustVisor in L1 also produces this bug.

Observation: this bug looks more reproducible when the XMHF boot sequence is
slow.

By debugging with KVM and GDB, see:
* CPU 6 does not enter PAL as expected
* CPU 6: `vcpu->vmcs.control_EPT_pointer = 0x1426501e`
  `<g_vmx_ept_pml4_table_buffers+30>`
	* PAL is marked as present in this EPT
* other: `vcpu->vmcs.control_EPT_pointer = 0x1425f01e`
  `<g_vmx_ept_pml4_table_buffers+6*4096+30>`
	* PAL is marked as not present in this EPT

This problem becomes more apparent by adding
`HALT_ON_ERRORCOND(hpt_emhf_get_l1_root_pm(vcpu) == g_reg_npmo_l1_root.pm);`
in `scode_register()` after changing EPTs to be the same. The assertion will
fail. See `xmhf64-nest-dev e4ee57cf1`.

#### TrustVisor Vulnerability 2

By debugging with GDB and QEMU's savevm and loadvm, the cause of this bug is
known:
* The intercept handler process is
	1. (Host RIP)
	2. `xmhf_baseplatform_arch_x86vmx_getVMCS()`
	3. Handle intercept
	4. `xmhf_baseplatform_arch_x86vmx_putVMCS()`
	5. VMRESUME
* Suppose CPU 0 is initiating quiesce (first call to `scode_register()`). CPU 1
  is between step 1 and 2.
* CPU 0 sets CPU 1's `vcpu->vmcs.control_EPT_pointer = common EPTP`
* CPU 1 at step 2 will set `vcpu->vmcs.control_EPT_pointer = CPU 1's EPTP`
* From now on EPTP of CPU 1 is wrong

During a real debug session, by dumping stack, can see CPU 1 is at
`<xmhf_baseplatform_arch_x86vmx_getVMCS+830>` (for VMCS field 0x2012) or
`<xmhf_baseplatform_arch_x86vmx_getVMCS+959>` (for VMCS field 0x2018). EPTP is
VMCS field encoding 0x201a. This result supports our hypothesis.

In theory, this race condition happens when nested virtualization is not
enabled. Not sure why this problem has not been catched earlier.

This should be considered another vulnerability of TrustVisor. An attacker can
use this race condition to make CPU 0 execute PAL and CPU 1 access PAL using
red os.

Future work: implement this exploit

Previous ideas
* Manually break at first occurrence of quiesce, then watch EPTP values
	* Exactly what we did above
* Also consider use savevm and loadvm
	* Exactly what we did above
* Check whether this is a regression
	* No

To fix this bug, there are a few ways
* Update VCPU to add a flag like `vcpu->epep01_changed`. After calling 
  `xmhf_baseplatform_arch_x86vmx_{get,put}VMCS()`, check this flag.
* Change the entries of the highest level to be the same (i.e. 512 PML4E's, but
  keep EPTPs different. This is a hacky workaround.
* Disallow changing EPTP01. Instead when changing each entry update EPT of all
  CPUs. This is more flexible and may benefit when implementing better shadow
  page table. This also removes the assumption of same MTRR across all CPUs.

After thinking more on this bug, I realized that the race condition is more
pervalent. Imagine a new feature for TrustVisor that does the following:
* When CPU 0 performs hyper call, steal physical page 0x12345000 from red EPT01
  and write secret info to this page.

Guest's access to memory can be considered atomic. However, when walking EPT in
software it is not. Race condition happens when
* CPU 1 initiates access to 0x12345000 (currently contains 0s), using guestmem
  interface
* guestmem almost completes walking EPT01, knows gpa 0x12345000 is spa
  0x12345000. Just before accessing spa 0x12345000
* CPU 0 performs hyper call, quiesce all other CPUs
* CPU 0 removes 0x12345000 from EPT01, write secret to spa 0x12345000
* CPU 0 ends quiesce
* CPU 1 accesses spa 0x12345000, gets the secret

This scenario can also happen in TrustVisor in theory. CPU 0 needs to end
quiesce, enter guest, enter scode (PAL), and write to the address. During this
time CPU 1 only executes a few address and does not reach accessing spa
0x12345000 yet.

To fix this bug, options are:
* Disallow quiescing during "critical section", meaning when walking EPT.
	* Con: other CPUs will be delayed while waiting for quiesce
* If EPT is changed, set a flag to require re-walking EPT.

Implemented changing PML4E's in `xmhf64 a6bb99deb`.

Implemented "set a flag" in `xmhf64 2faecda78`.

Fixed a few bugs in `xmhf64 2faecda78..948445fcd`.

Merged to development code in `xmhf64-nest-dev 09f5d0ed2`.

In `xmhf64-nest-dev 09f5d0ed2..466e80ea1`, fixed a bug and implemented
`tv_app_handle_ept02_change()`. However, noticed design issue in EPT interface.
See below.

### Need for redesigning EPT changing code

We should redesign how hypapp is able to change EPT and the discernment of L1
and L2.

The trigger of this analysis is that `_rewalk_ept01_control_EPT_pointer()`
requires complicated call to TrustVisor during scode. TrustVisor will save red
EPT02 and red EPT12, and replace them with green EPT02 and green EPT12=invalid.
When rewalking, current implementation need to:
* Call TrustVisor to get saved red EPT12
* Compute EPT02 cache line
* Call TrustVisor to store computed red EPT02 and get green EPT02
* Store green EPT02 to VMCS

This process is complicated and not flexible. I think the problem is that EPT02
is cache and should not be managed by TrustVisor. Instead, TrustVisor should
modify EPT01, and XMHF needs to compute EPT02 according to the modification.
For TrustVisor, the logic is easy since the only combinations are:
* EPT01 = red, EPT12 = invalid (red L1)
* EPT01 = red, EPT12 = red guest (red L2)
* EPT01 = pal, EPT12 = invalid (green L1)

The logic is generally easy when EPT12 = invalid. At that time EPT02 = EPT01.

Future work
* Review TrustVisor use of INVEPT. There are likely unnecessary calls
* Try to support INVEPT of specific EPT01, not all EPTs
* For EPT02 cache, tag with EPT01 and EPT12 (currently only EPT12)

### Hyper-V BSOD

At `xmhf64-nest-dev cd31b87f1`, when running XMHF, Hyper-V, Windows 10, L2 PAL
demo, see blue screen of death. When registering PAL for a long time, still see
EPT violation (likely antivirus)

Build XMHF with
```
./build.sh amd64 --mem 0x230000000 fast circleci O3 --ept-num 3 --ept-pool 1024 --event-logger && gr
```

BSOD reason code:
* `SYSTEM_SERVICE_EXCEPTION` (previous run, 3 times)
* `KERNEL_MODE_HEAP_CORRUPTION`
* `CRITICAL_PROCESS_DIED`
* `SYSTEM_SERVICE_EXCEPTION`
* `HYPERVISOR_ERROR`, serial `20221128124908`

BSOD address using BlueScreenView: `ntoskrnl.exe+3f71b0`.

VMMap also looks useful when dealing with the antivirus problem, but not useful
for now.

Looks like running `./test_args32L2 1 * *` and `./test_args32L2 2 * *`.
However, running `./test_args32L2 4 * *` soon triggers the problem. Behavior of
`test_args64L2` is similar.

After removing call to PAL in `test_args32L2`, the blue screen is still
reproducible. So we guess that the bluescreen happens when PAL register /
unregister happens frequently.

From current observed behavior, looks like registering / unregistering PALs
causes memory corruption in Windows. Then as a result Windows blue screens. I
think actions to EPTs are still the most suspicious.

TODO: What problem will happen if running pal_demo in Hyper-V virtual machine (e.g. Debian)
TODO: register scode slowly (1 second / vmcall), see whether bug still reproducible

### VirtualBox SMP problem

Running virtualbox with one CPU works well. However, when I set it to 4 CPUs,
after Debian 11 x64 in VirtualBox in Debian in XMHF boots, see XMHF assertion
error:
```
HPT[3]:xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_get_pmeo:398: EU_VERIFY( hpt_pmeo_is_page(pmeo)) failed
```

This problem is not reproducible in `xmhf64-nest ef6432ebe`. So this is a
regression.

Afterwards: can no longer reproduce this problem in `xmhf64-nest-dev cd31b87f1`.
This is strange. Since cannot reproduce, go ahead and test others first.

Ideas:
* Since KVM it too slow, get a stack trace using ud2
* Try to reproduce in KVM?
	* However, "KVM, Virtualbox" is already slow / not stable

While experimenting on Hyper-V, also see `EU_VERIFY( hpt_pmeo_is_page(pmeo))`
exception. Stack dump is in `20221128234304`. It is hard to trace stack because
compiled in 64-bit and O3 optimization. Guessed call stack is:
```
xmhf_parteventhub_arch_x86vmx_entry
	?: xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception
	?: _update_nested_nmi
	xmhf_parteventhub_arch_x86vmx_intercept_handler
		xmhf_nested_arch_x86vmx_handle_vmexit
			handle_vmexit20_ept_violation
				xmhf_nested_arch_x86vmx_handle_ept02_exit
					xmhf_nested_arch_x86vmx_walk_ept02
						hptw_checked_get_pmeo (first call, on EPT12)
							(hptw_checked_copy_from_va)
							(hpt_pmeo_va_to_pa)
							(hptw_checked_access_va)
							(hpt_pmeo_va_to_pa)
							abort
```

Looks like it is hard (low probability) to reproduce this bug. So for now just
add more print debugging before `abort()` and work on the other bug.

TODO: add more print debugging if VERIFY will return false in `hptw_checked_get_pmeo`

TODO: for debugging, do not remove the page from regular EPT. Just add to pal EPT
TODO: randomly disable Windows security features
TODO: let TV display process memory space

TODO
TODO: test on Windows Hyper-V
TODO: redesign EPT iface, see `### Need for redesigning EPT changing code`
TODO: `#### TrustVisor Vulnerability 1` above
TODO: `#### TrustVisor Vulnerability 2` above

