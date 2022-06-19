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

In git `c13fa1e94`, implemented most MSR load/store. Can run well on one CPU.

### Quiescing deadlock

When running git `c13fa1e94` on multiple CPUs, can easily see deadlock in
quiescing. Quiescing happens because MTRR write results in quiescing.

First make sure to remove printing debug messages in MTRR
```diff
diff --git a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx.c b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx.c
index fb53653c0..93d159009 100755
--- a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx.c
+++ b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx.c
@@ -432,8 +432,8 @@ u32 xmhf_memprot_arch_x86vmx_mtrr_write(VCPU *vcpu, u32 msr, u64 val) {
        {
                u64 oldval;
                HALT_ON_ERRORCOND(xmhf_memprot_arch_x86vmx_mtrr_read(vcpu, msr, &oldval) == 0);
-               printf("CPU(0x%02x): WRMSR (MTRR) 0x%08x 0x%016llx (old = 0x%016llx)\n",
-                               vcpu->id, msr, val, oldval);
+//             printf("CPU(0x%02x): WRMSR (MTRR) 0x%08x 0x%016llx (old = 0x%016llx)\n",
+//                             vcpu->id, msr, val, oldval);
        }
        /* Check whether hypapp allows modifying MTRR */
        xmhf_smpguest_arch_x86vmx_quiesce(vcpu);
```

Sample stacks trace in GDB are:

```
(gdb) bt
#0  0x1021ed57 in xmhf_smpguest_arch_x86vmx_quiesce (vcpu=0x1025bb40 <g_vcpubuffers>) at arch/x86/vmx/smpg-x86vmx.c:562
#1  0x10204cc8 in xmhf_memprot_arch_x86vmx_mtrr_write (vcpu=0x1025bb40 <g_vcpubuffers>, msr=524, val=3435970560) at arch/x86/vmx/memp-x86vmx.c:439
#2  0x102073e9 in xmhf_parteventhub_arch_x86vmx_handle_wrmsr (vcpu=0x1025bb40 <g_vcpubuffers>, index=524, value=3435970560) at arch/x86/vmx/peh-x86vmx-main.c:450
#3  0x1021a3ea in xmhf_nested_arch_x86vmx_vmcs12_to_vmcs02 (vcpu=0x1025bb40 <g_vcpubuffers>, vmcs12_info=0x102916e0) at arch/x86/vmx/nested-x86vmx-vmcs12.c:640
#4  0x1020a3b5 in _vmx_vmentry (vcpu=0x1025bb40 <g_vcpubuffers>, vmcs12_info=0x102916e0, r=0x1d0d5fdc <g_cpustacks+16348>) at arch/x86/vmx/nested-x86vmx-handler.c:361
#5  0x1020b0bc in xmhf_nested_arch_x86vmx_handle_vmlaunch_vmresume (vcpu=0x1025bb40 <g_vcpubuffers>, r=0x1d0d5fdc <g_cpustacks+16348>, is_vmresume=1) at arch/x86/vmx/nested-x86vmx-handler.c:654
#6  0x10208559 in xmhf_parteventhub_arch_x86vmx_intercept_handler (vcpu=0x1025bb40 <g_vcpubuffers>, r=0x1d0d5fdc <g_cpustacks+16348>) at arch/x86/vmx/peh-x86vmx-main.c:1183
#7  0x10206ab7 in xmhf_parteventhub_arch_x86vmx_entry () at arch/x86/vmx/peh-x86vmx-entry.S:86
#8  0x1025bb40 in g_cpumap ()
#9  0x1d0d5fdc in g_cpustacks ()
#10 0x00000000 in ?? ()
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
(gdb) t 2
[Switching to thread 2 (Thread 1.2)]
#0  spin_lock () at arch/x86/bplt-x86-i386-smplock.S:58
58	      	jnc	spin			      //wait till it is
(gdb) bt
#0  spin_lock () at arch/x86/bplt-x86-i386-smplock.S:58
#1  0x1021ed08 in xmhf_smpguest_arch_x86vmx_quiesce (vcpu=0x1025c058 <g_vcpubuffers+1304>) at arch/x86/vmx/smpg-x86vmx.c:544
#2  0x10204cc8 in xmhf_memprot_arch_x86vmx_mtrr_write (vcpu=0x1025c058 <g_vcpubuffers+1304>, msr=523, val=3149639680) at arch/x86/vmx/memp-x86vmx.c:439
#3  0x102073e9 in xmhf_parteventhub_arch_x86vmx_handle_wrmsr (vcpu=0x1025c058 <g_vcpubuffers+1304>, index=523, value=3149639680) at arch/x86/vmx/peh-x86vmx-main.c:450
#4  0x1021cbeb in xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12 (vcpu=0x1025c058 <g_vcpubuffers+1304>, vmcs12_info=0x10294660) at arch/x86/vmx/nested-x86vmx-vmcs12.c:990
#5  0x1020abe5 in xmhf_nested_arch_x86vmx_handle_vmexit (vcpu=0x1025c058 <g_vcpubuffers+1304>, r=0x1d0d9fdc <g_cpustacks+32732>) at arch/x86/vmx/nested-x86vmx-handler.c:553
#6  0x10208306 in xmhf_parteventhub_arch_x86vmx_intercept_handler (vcpu=0x1025c058 <g_vcpubuffers+1304>, r=0x1d0d9fdc <g_cpustacks+32732>) at arch/x86/vmx/peh-x86vmx-main.c:1090
#7  0x10206ab7 in xmhf_parteventhub_arch_x86vmx_entry () at arch/x86/vmx/peh-x86vmx-entry.S:86
#8  0x1025c058 in g_vcpubuffers ()
#9  0x1d0d9fdc in g_cpustacks ()
#10 0x00000001 in ?? ()
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
(gdb) 
```

There is always 1 CPU (CPU A) waiting on the quiesce spin lock and another
(CPU B) waiting for CPU A to quiesce (B sends A an interrupt).

Looks like the problem is not related to NMI blocking. When CPU 0 is stuck on
the spin lock, sending NMI in QEMU can let the system make some progress.
However when CPU 1 is stuck, sending NMI does not help.

I guess the cause of this problem is quiescing frequently. We need to check
the quiescing logic in `xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception()`.

TODO: make sure to uncomment "WRMSR (MTRR)" message
TODO: try to reproduce this problem independently
TODO: think about logic in `xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception()`.

