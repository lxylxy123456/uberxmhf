# Support guest hypervisor setting MSR and EPT

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `d3c97a24b`
* Git branch `lhv` commit `d90b70785`

## Behavior
Currently for the guest hypervisor running in XMHF, EPT is not supported.
MSR load / store count must be 0. These need to be changed in order to run
unrestricted nested guests.

## Debugging

### Support MSR load / store

In LHV `d90b70785..fb6b251d6`, added code to load MSR and check whether the
add is successful. Need to build with `--lhv-opt 1`. The MSRs used to test are
unused MTRRs.

Note: MSR load / store area can be larger than 1 page. Be careful.

We need to consider 3 types of MSR accesses
* VMENTRY
	* Load L2 MSRs
* VMEXIT
	* Store L2 MSRs
	* Load L1 MSRs

All MSRs specially treated in `_vmx_handle_intercept_rdmsr()` also need to
be treated specially here.

We also need to always switch all MSRs `vmx_msr_area_msrs` always.

There are 3 types of MSRs
* EFER-like: 3 MSRs in `vmx_msr_area_msrs`, always switch
* MTRR-like: MSRs handled in `_vmx_handle_intercept_rdmsr()`
* normal: shared by xmhf and guest

We discuss all cases
* VMENTRY load
	* EFER-like: if requested, load what is requested by L1; else, load L1's value
	* MTRR-like: behave as if L1 performs WRMSR (side effect goes to L1)
	* normal: load what is requested by L1
* VMEXIT store
	* EFER-like: if requested, store what is requested by L1
	* MTRR-like: behave as if L1 performs RDMSR
	* normal: store what is requested by L1
* VMEXIT load
	* EFER-like: always load L0's value
	* MTRR-like: behave as if L1 performs WRMSR
	* normal: load what is requested by L1

So apart from MTRR, we need something else to test normal MSRs.

KVM's implementation is in `nested_vmx_load_msr()` in `nested.c`. Looks like
it simulates WRMSR for each MSR manually. i.e. if the VMCS12's VMENTRY load
count is 100, the VMCS02's load count is still 3. The around 97 MSRs are loaded
manually. We should follow this approach.

Note that if not using this approach, there are ways to detect hypervisor
presence. Suppose the MSR loads are:
* MTRR1
* 0xdead
* MTRR2

The hardware will only load MTRR1, but a hypervisor is likely to load both or
none.

TODO: follow the QEMU approach

