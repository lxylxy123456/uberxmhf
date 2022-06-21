# Support guest hypervisor setting MSR

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `d3c97a24b`
* Git branch `lhv` commit `d90b70785`

## Behavior
Currently for the guest hypervisor running in XMHF, MSR load / store count must
be 0. This need to be changed in order to run unrestricted nested guests.

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

In `0d6d0092b`, remove some if branches that we do not consider.

The quiesce code is in `smpg-x86vmx.c`. Pseudo code is
```
xmhf_smpguest_arch_x86vmx_quiesce() {
	lock g_vmx_lock_quiesce
	lock emhfc_putchar_linelock_arg
	vcpu->quiesced = 1
	lock g_vmx_lock_quiesce_counter
	g_vmx_quiesce_counter = 0
	unlock g_vmx_lock_quiesce_counter
	g_vmx_quiesce = 1
	send NMI to other CPUs
	while (g_vmx_quiesce_counter < nproc - 1) {
		continue
	}
	unlock emhfc_putchar_linelock_arg
}

xmhf_smpguest_arch_x86vmx_endquiesce() {
	g_vmx_quiesce = 0
	g_vmx_quiesce_resume_counter = 0
	g_vmx_quiesce_resume_signal = 1
	while (g_vmx_quiesce_resume_counter < nproc - 1) {
		continue
	}
	vcpu->quiesced = 0
	lock g_vmx_lock_quiesce_resume_signal
	g_vmx_quiesce_resume_signal = 0
	unlock g_vmx_lock_quiesce_resume_signal
	unlock g_vmx_lock_quiesce
}

xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception() {
	if (g_vmx_quiesce && !vcpu->quiesced) {
		vcpu->quiesced=1;
		lock(&g_vmx_lock_quiesce_counter)
		g_vmx_quiesce_counter++
		unlock(&g_vmx_lock_quiesce_counter)
		while(!g_vmx_quiesce_resume_signal) {
			continue;
		}
		lock g_vmx_lock_quiesce_resume_counter
		g_vmx_quiesce_resume_counter++
		unlock g_vmx_lock_quiesce_resume_counter
		vcpu->quiesced = 0
	} else {
		xmhf_smpguest_arch_x86vmx_inject_nmi(vcpu);
	}
	if (from_guest) {
		xmhf_smpguest_arch_x86vmx_unblock_nmi();
	}
}
```

In git `e1b6d75bf`, use `nmi_log` to track whether NMI happens in vmx root mode
or non-root mode. Looks like most cases NMI happens in vmx root mode (xmhf).
For example:
```
(gdb) p nmi_log
$2 = {"\001\002\001\001\001", '\000' <repeats 9994 times>, "\001\001\001\001\001\001", '\000' <repeats 9993 times>}
(gdb) 
```

Sometimes there are only `1`, but still deadlock.

In git `0e5dfd145`, we also make sure that the nested guest does not receive
NMI.

Until now, I have been using lhv at git `15ec1941e` with lhv-opt 1.

We should try to reproduce this without nested virtualization. First use
lhv-opt 0, and manually add MTRR writes.

It is difficult to reproduce this problem without entering nested guest. I
guess the problem is that the nested guest does not enable virtual NMI. Should
enable it as in `d96163ddf..831ea9845`. Then will see exception exit due to NMI
from nested guest. This is not handled yet.

After setting enable virtual NMI, looks like the problem only happens when
nested guest receives NMI. xmhf git `74e6b36c0`, lhv git `0aec8be25`,
lhv-opt 0, collect logs using:

```
while sleep 30; do
	timeout 15 ./bios-qemu.sh --gdb 2198       -d build32 +1 -d build32lhv +4 --qb32  -smp 2 -display none | tee /tmp/`date +%Y%m%d%H%M%S`
done
```

The logs show that all deadlocks happen after seeing
```
Fatal: Halting! Condition '0 && "Nested guest NMI handling not implemented"' failed, line 593, file arch/x86/vmx/nested-x86vmx-handler.c
```

So we just need to implement nested guest NMI handling.

In git `ff69c073a`, call the NMI handler when guest has VMEXIT due to NMI.
Looks like problem solved. However, need to refactor XMHF code.

The NMI problem is fixed in git `c6f552b61`.

### NMI not quite fixed

For some reason we thought that the problem is fixed. But actually, it is not.

Reproducible xmhf `94b5e751a`, lhv `94b5e751a`, lhv-opt `1`.

Maybe the problem is related to NMI blocking. When 

Reproducible xmhf `eefe48737`, lhv `9fc27ae29`, lhv-opt `1`, see
`global_bad = 0`. That means, the nested guest has not received NMIs, and all
NMIs are decided to be for quiescing.

A difference for this time is that when typing `nmi` in QEMU monitor, the CPU
stuck no longer proceeds. So there is probably some problem with hardware NMI
blocking.

Also, a strange thing is that if we skip entering the guest, the problem is no
longer reproducible. For example, in git `9e04ae245`, if compile with
`SKIP_NESTED_GUEST` defined, the problem is no longer reproducible.

Also, a sample stack trace is below.
```
(gdb) bt
#0  0x1021ef3d in xmhf_smpguest_arch_x86vmx_quiesce (vcpu=0x1025bb40 <g_vcpubuffers>) at arch/x86/vmx/smpg-x86vmx.c:562
#1  0x10204d01 in xmhf_memprot_arch_x86vmx_mtrr_write (vcpu=0x1025bb40 <g_vcpubuffers>, msr=523, val=3149639680) at arch/x86/vmx/memp-x86vmx.c:439
#2  0x10207422 in xmhf_parteventhub_arch_x86vmx_handle_wrmsr (vcpu=0x1025bb40 <g_vcpubuffers>, index=523, value=3149639680) at arch/x86/vmx/peh-x86vmx-main.c:450
#3  0x1021cd7f in xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12 (vcpu=0x1025bb40 <g_vcpubuffers>, vmcs12_info=0x10291860) at arch/x86/vmx/nested-x86vmx-vmcs12.c:998
#4  0x1020ac1e in xmhf_nested_arch_x86vmx_handle_vmexit (vcpu=0x1025bb40 <g_vcpubuffers>, r=0x1d0d5fdc <g_cpustacks+16348>) at arch/x86/vmx/nested-x86vmx-handler.c:562
#5  0x1020833f in xmhf_parteventhub_arch_x86vmx_intercept_handler (vcpu=0x1025bb40 <g_vcpubuffers>, r=0x1d0d5fdc <g_cpustacks+16348>) at arch/x86/vmx/peh-x86vmx-main.c:1090
#6  0x10206af0 in xmhf_parteventhub_arch_x86vmx_entry () at arch/x86/vmx/peh-x86vmx-entry.S:86
(gdb) t 2
[Switching to thread 2 (Thread 1.2)]
#0  spin_lock () at arch/x86/bplt-x86-i386-smplock.S:57
57	    spin:	btl	$0, (%esi)		//mutex is available?
(gdb) bt
#0  spin_lock () at arch/x86/bplt-x86-i386-smplock.S:57
#1  0x102590ca in emhfc_putchar_linelock (arg=0x1025fa60) at xmhfc-putchar.c:62
#2  0x102437e8 in vprintf (fmt=0x10260c98 "CPU(0x%02x): WRMSR (MTRR) 0x%08x 0x%016llx (old = 0x%016llx)\n", ap=0x1d0d9c68 <g_cpustacks+31848>) at /home/lxy/文档/GreenBox/build32/xmhf/src/libbaremetal/libxmhfc/subr_prf.c:345
#3  0x102437cd in printf (fmt=0x10260c98 "CPU(0x%02x): WRMSR (MTRR) 0x%08x 0x%016llx (old = 0x%016llx)\n") at /home/lxy/文档/GreenBox/build32/xmhf/src/libbaremetal/libxmhfc/subr_prf.c:316
#4  0x10204cf6 in xmhf_memprot_arch_x86vmx_mtrr_write (vcpu=0x1025c058 <g_vcpubuffers+1304>, msr=523, val=3149639680) at arch/x86/vmx/memp-x86vmx.c:435
#5  0x10207422 in xmhf_parteventhub_arch_x86vmx_handle_wrmsr (vcpu=0x1025c058 <g_vcpubuffers+1304>, index=523, value=3149639680) at arch/x86/vmx/peh-x86vmx-main.c:450
#6  0x1021cd7f in xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12 (vcpu=0x1025c058 <g_vcpubuffers+1304>, vmcs12_info=0x102947e0) at arch/x86/vmx/nested-x86vmx-vmcs12.c:998
#7  0x1020ac1e in xmhf_nested_arch_x86vmx_handle_vmexit (vcpu=0x1025c058 <g_vcpubuffers+1304>, r=0x1d0d9fdc <g_cpustacks+32732>) at arch/x86/vmx/nested-x86vmx-handler.c:562
#8  0x1020833f in xmhf_parteventhub_arch_x86vmx_intercept_handler (vcpu=0x1025c058 <g_vcpubuffers+1304>, r=0x1d0d9fdc <g_cpustacks+32732>) at arch/x86/vmx/peh-x86vmx-main.c:1090
#9  0x10206af0 in xmhf_parteventhub_arch_x86vmx_entry () at arch/x86/vmx/peh-x86vmx-entry.S:86
(gdb) 
```

In this example, CPU 1 (thread 2) should receive NMI from CPU 0. Note that CPU
1 is handling VMEXIT from L2 guest (in contrast to L1 guest). This seems to be
a pattern.

Due to the clues above, we guess that when L2 guest VMEXITs, NMI is blocked for
L0 for mysterious reasons. However, in git `80dd63976`, adding
`xmhf_smpguest_arch_x86vmx_unblock_nmi()` does not solve the problem. However,
removing `xmhf_parteventhub_arch_x86vmx_handle_wrmsr()` in
`xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()` makes the problem disappear.

To reduce the code, we try to print things instead of quiescing in
`vmcs02_to_vmcs12`. In git `795bc9113` (lhv git `8b7e466fd`, option 1), remove
wrmsr code in `vmcs02_to_vmcs12`, an still see reproducible.

I think at last we may be comparing L1's VMCS and L2's VMCS and see what causes
the NMI blocking.

In `33557891a`, able to reproduce without MTRRs (i.e. use lhv `8b7e466fd`,
lhv-opt 0).

Also reproducible in xmhf `33557891a`, lhv `97894439f`, lhv-opt 0. This commit
changes LHV to simplify hypervisor logic. The VMEXIT handler does nothing and
simply runs VMRESUME.

In xmhf git `4debecdd8`, skip converting between VMCS02 and VMCS12 to speed up
reproducing.

We also notice that the hacky fix `xmhf_smpguest_arch_x86vmx_unblock_nmi()` in
`xmhf_nested_arch_x86vmx_handle_vmexit()` is effective. When this line is
added, QEMU responds to `nmi` in QEMU monitor. When this line is removed, QEMU
does not respond.

I think this indicates that entering L2 guest probably does something strange.
It looks like NMIs are blocked and probably one NMI is lost.

In `b28a25d0a`, serial `20220620112939`, collect VMCS dump. LHV image can be
found in `lhv-97894439f.img.xz`.

The logic is
* Guest HV requests VMENTRY
* Collect VMENTRY01
* VMPTRLD VMCS02
* Edit VMCS02
* Collect VMENTRY02
* VMLAUNCH / VMRESUME
* Nested guest causes VMEXIT
* Collect VMEXIT02
* VMPTRLD VMCS01
* Edit VMCS01
* Collect VMEXIT01
* VMRESUME

For example, in the following 3 outputs, the last 2 are the same. The first
2 are similar (only off by VMEXIT reason fields)

```sh
cat 20220620112939 | grep 'CPU(0x00):' | grep VMEXIT01 | head -n 167
cat 20220620112939 | grep 'CPU(0x00):' | grep VMEXIT01 | tail -n +168 | head -n 167
cat 20220620112939 | grep 'CPU(0x00):' | grep VMEXIT01 | tail -n +168 | tail -n +168 | head -n 167
```

Across multiple CPUs, there are some more changes.
```sh
cat 20220620112939 | grep 'CPU(0x00):' | grep VMEXIT01 | tail -n +168 | head -n 167 | cut -b 10-
cat 20220620112939 | grep 'CPU(0x01):' | grep VMEXIT01 | tail -n +168 | tail -n +168 | head -n 167 | cut -b 10-
```

Here is a program to extract the dumps
```py
import re
vmexit01 = [[], []]
vmexit02 = [[], []]
vmentry01 = [[], []]
vmentry02 = [[], []]
d = {'VMEXIT': {'01': vmexit01, '02': vmexit02},
	'VMENTRY': {'01': vmentry01, '02': vmentry02}}
for i in open('20220620112939').read().split('\n'):
	matched = re.fullmatch('CPU\(0x(\d+)\): \((0x[0-9a-f]{4})\) (VMEXIT|VMENTRY)(0[12])(\w+) = (0x[0-9a-f]+|unavailable)', i)
	if not matched:
		continue
	cpu, enc, entry_exit, _01_02, name, value_str = matched.groups()
	l = d[entry_exit][_01_02][int(cpu)]
	if not l or len(l[-1]) == 167:
		l.append([])
	l[-1].append((enc, name, value_str))

for ki, vi in d.items():
	for kj, vj in vi.items():
		for kk, vk in enumerate(vj):
			for kl, vl in enumerate(vk):
				if len(vl) < 167:
					continue
				fname = 'tmp/%s%s_CPU%02d_%d' % (ki, kj, kk, kl)
				with open(fname, 'w') as f:
					for i in vl:
						print('(%s) %s = %s' % i, file=f)
```

We can see that `VMEXIT01_CPU(i)_(n) = VMENTRY01_CPU(i)_(n)`, for `n >= 1`.
Also `VMEXIT02_CPU(i)_(n) = VMENTRY02_CPU(i)_(n+1)`, for `n >= 0`. These are
all expected. We can use these information to hard-code VMCS in XMHF. The
relevant addresses in the guest are

```
(gdb) x/i 0x0820596f
   0x820596f:	vmresume
(gdb) x/i 0x0820b7cf
   0x820b7cf:	vmcall
(gdb) 
```

`diff VMENTRY01_CPU00_2 VMENTRY02_CPU00_2` shows differences between VMCS01 and
VMCS02. The control fields are the following. However, there are not a lot of
clues.

```
67c67
< (0x4002) control_VMX_cpu_based = 0x86006172
---
> (0x4002) control_VMX_cpu_based = 0x8401e172
72c72
< (0x400c) control_VM_exit_controls = 0x00036dfb
---
> (0x400c) control_VM_exit_controls = 0x00036dff
75c75
< (0x4012) control_VM_entry_controls = 0x000011fb
---
> (0x4012) control_VM_entry_controls = 0x000011ff
81c81
< (0x401e) control_VMX_seccpu_based = 0x000010aa
---
> (0x401e) control_VMX_seccpu_based = 0x00000002
```

We can use the Python code to generate code to write VMCS. Be careful that
MSR, EPT, and many host state fields are pointers that may change after XMHF
code changes.

```py
for e, n, v in vmentry02[0][1]:
	if v == 'unavailable':
		continue
	s = {'0': '16', '2': '64', '4': '32', '6': 'NW'}[e[2]]
	print('\t__vmx_vmwrite%s(%s, %s); /* %s */' % (s, e, v, n))

# Do the same thing for vmexit01[0][1]
```

In git `2617b040a`, able to remove a lot of non-static code that modifies VMCS.
Basically peh no longer handles intercepts. Instead, all intercepts happen in
nested handlers. The nested handlers have not implemented NMI exit handling, so
encountering such a VMEXIT will set `global_bad`. The reporducibility criteria
is:
* Thing stops to be printed in serial
* All CPUs should be running (i.e. not HLT)
* In gdb, `global_bad = 0`
* In gdb, CPU 0 should be trying to lock printf lock, CPU 1 should be waiting
  for CPU 0 to quiesce

In `8992699fe`, realized that a `global_bad` is missing, cause false positives
in things thought to be reproducible.

```
* 8992699fe Add global_bad
* 2617b040a Support NO_VMPTRLD												bad
* eb3f815ae Forward everything to xmhf_nested_arch_x86vmx_handle_vmexit()
* 9e8959112 Remove guest NMI blocking										bad
* 23855fa38 global_bad everywhere
* a18b9ef6f Remove more code
* 58a6ce80d Remove vmcs12 and vmcs02 translation
* 9c331b3ba Control VMCS01													skip
* 731bc77aa Set VMCS01 state
* 45d9251e0 Fix assertion error												skip
* e5bf072fd Complete take control of VMCS02
* 03caf80de Still reproduce by hardcoding most of VMCS02					FIND
* b28a25d0a Collect VMCS dump
```

At `03caf80de`, found that the reproducing code is incorrect. In
`xmhf_nested_arch_x86vmx_handle_vmexit()`, before handling NMI exit
(`xmhf_smpguest_arch_x86vmx_nmi_check_quiesce()`), the print "hello" is called.
This of course causes deadlock when nested guest receives NMI intercept.

`03caf80de..896612547` solves the problem by moving print "hello" to after
handling NMI. Now the reproducing should be legit.

`8ebfba40d` merges the changes from `896612547` and debugged. However, it no
longer reproduces the bug.

In `ad4e34aae`, found that adding some code to use `__vmx_vmread32(0x4402)` to
check whether the current VMEXIT is for NMI instead of using
`vmcs12_info->vmcs12_value.info_vmexit_reason` solves the problem. Try to
backport it to `xmhf64-nest` and see whether it solves the problem.

Indeed, this solves the problem. See git `c6f552b61..296d39b8d`.

Untried ideas:
* check NMI related fields in VMCS02
* use correct sync primitives in NMI exception handler
* try on another machine
* `NO_VMPTRLD`
* try something other than KVM

### Why is there a problem?

The old logic of `xmhf_nested_arch_x86vmx_handle_vmexit()` is:
* Call `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`
* Check `vmcs12_info` (modified by `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`)
  to see whether NMI happens

Unfortunately `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()` prints to serial
port. However, before NMI is handled printing to serial port will deadlock.

### Bug in handling MSRs that the host cares about

In lhv `36faf216d`, tested the EFER-like MSRs. The XMHF's behavior is incorrect
when dealing with VMENTRY MSR load. If the corresponding MSR is not present
in VMEXIT MSR load. Fixed in xmhf `24fbc0c2d`.

### Testing normal MSRs

The biggest challenge in testing normal MSRs is to find MSRs that can be
written to. In lhv-dev `b4a2e906b`, use safe MSR to scan all MSRs. See results
below.

```
MSR        Value read         Bit can write      Bit will preserve  Comment
0x00000000 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000001 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000010 0x000000005732c2d0 0xffffffffffffffff 0x0000000000000000
0x00000011 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000012 0x00000000000e8f61 0xffffffffffffffff 0xffffffffffffffff
0x00000017 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000001b 0x00000000fee00900 0x000000007ffffd00 0x000000007ffffd00
0x0000002a 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000002c 0x0000000001000000 0x0000000000000000 0x0000000000000000
0x00000034 0x000000000000000b 0x0000000000000000 0x0000000000000000
0x0000003a 0x0000000000000005 0x0000000000000000 0x0000000000000000
0x0000003b 0xffffffffffe4bdde 0xffffffffffffffff 0x0000000000000000
0x0000008b 0x0000000100000000 0xffffffffffffffff 0x0000000000000000
0x000000c1 0x0000000000000000 0xffffffffffffffff 0x0000000000000000
0x000000c2 0x0000000000000000 0xffffffffffffffff 0x0000000000000000
0x000000cd 0x0000000000000003 0x0000000000000000 0x0000000000000000
0x000000ce 0x0000000080000000 0x0000000000000000 0x0000000000000000
0x000000fe 0x0000000000000508 0x0000000000000000 0x0000000000000000
0x0000011e 0x00000000be702111 0xffffffffffffffff 0x0000000000000000
0x00000140 0x0000000000000000 0x0000000000000001 0x0000000000000001
0x00000174 0x0000000000000000 0xffffffffffffffff 0x000000007fffffff IA32_SYSENTER_CS
0x00000175 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff IA32_SYSENTER_ESP
0x00000176 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff IA32_SYSENTER_EIP
0x00000179 0x000000000100010a 0x0000000000000000 0x0000000000000000
0x0000017a 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff IA32_MCG_STATUS
0x0000017b 0xffffffffffffffff 0x0000000000000000 0x0000000000000000
0x00000186 0x0000000000000000 0xffffffffffffffff 0x0000000000000000
0x00000187 0x0000000000000000 0xffffffffffffffff 0x0000000000000000
0x00000198 0x00000400000003e8 0x0000000000000000 0x0000000000000000
0x00000199 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000001a0 0x0000000000000001 0xffffffffffffffff 0xffffffffffffffff
0x000001d9 0x0000000000000000 0x0000000000000003 0x0000000000000000
0x000001db 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000001dc 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000001dd 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000001de 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000001fc 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000200 0x0000000080000000 0x000000007ffff005 0x000000007ffff005
0x00000201 0x0000000f80000800 0x000000007ffff800 0x000000007ffff800
0x00000202 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x00000203 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x00000204 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x00000205 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x00000206 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x00000207 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x00000208 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x00000209 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x0000020a 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x0000020b 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x0000020c 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x0000020d 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x0000020e 0x0000000000000000 0x000000007ffff005 0x000000007ffff005
0x0000020f 0x0000000000000000 0x000000007ffff800 0x000000007ffff800
0x00000250 0x0606060606060606 0x0000000002020202 0x0000000002020202
0x00000258 0x0606060606060606 0x0000000002020202 0x0000000002020202
0x00000259 0x0000000000000000 0x0000000005050505 0x0000000005050505
0x00000268 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x00000269 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026a 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026b 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026c 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026d 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026e 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x0000026f 0x0505050505050505 0x0000000005050505 0x0000000005050505
0x00000277 0x0007040600070406 0x0000000005030703 0x0000000005030703
0x000002ff 0x0000000000000c06 0x0000000000000c02 0x0000000000000c02
0x00000402 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000403 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000406 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000407 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000040a 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000040b 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000040e 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000040f 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000412 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000413 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000416 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000417 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000041a 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000041b 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000041e 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x0000041f 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000422 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000423 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000426 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000427 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0x00000480 0x00d8100011e57ed0 0x0000000000000000 0x0000000000000000
0x00000481 0x0000007f00000016 0x0000000000000000 0x0000000000000000
0x00000482 0xfff9fffe0401e172 0x0000000000000000 0x0000000000000000
0x00000483 0x007ffdff00036dff 0x0000000000000000 0x0000000000000000
0x00000484 0x0000f3ff000011ff 0x0000000000000000 0x0000000000000000
0x00000485 0x0000000020000065 0x0000000000000000 0x0000000000000000
0x00000486 0x0000000080000021 0x0000000000000000 0x0000000000000000
0x00000487 0x00000000ffffffff 0x0000000000000000 0x0000000000000000
0x00000488 0x0000000000002000 0x0000000000000000 0x0000000000000000
0x00000489 0x00000000001727ff 0x0000000000000000 0x0000000000000000
0x0000048a 0x000000000000002e 0x0000000000000000 0x0000000000000000
0x0000048b 0x000078ff00000000 0x0000000000000000 0x0000000000000000
0x0000048c 0x00000f0106334041 0x0000000000000000 0x0000000000000000
0x0000048d 0x0000007f00000016 0x0000000000000000 0x0000000000000000
0x0000048e 0xfff9fffe04006172 0x0000000000000000 0x0000000000000000
0x0000048f 0x007ffdff00036dfb 0x0000000000000000 0x0000000000000000
0x00000490 0x0000f3ff000011fb 0x0000000000000000 0x0000000000000000
0x00000491 0x0000000000000001 0x0000000000000000 0x0000000000000000
0x00000606 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000611 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000619 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000639 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x00000641 0x0000000000000000 0x0000000000000000 0x0000000000000000
0x000006e0 0x0000000000000000 0xffffffffffffffff 0x0000000000000000
0xc0000080 0x0000000000000000 0x0000000000000801 0x0000000000000801
0xc0000081 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000082 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000083 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000084 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000100 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000101 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000102 0x0000000000000000 0xffffffffffffffff 0xffffffffffffffff
0xc0000103 0x0000000000000000 0x000000007fffffff 0x000000007fffffff
```

We can use MSRs near `0x00000402`. Implemented the test in lhv `e6b0d5183`.
Running on XMHF nest looks good.

## Fix

`d3c97a24b..24fbc0c2d`
* Support MSR load / store in VMEXIT / VMENTRY
* Indent nested virtualization code
* Support handling quiesce NMI in nested guest

