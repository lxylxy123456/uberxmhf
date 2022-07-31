# Running SMP Linux in XMHF in XMHF (Solve NMI problems)

## Scope
* KVM
* 32-bit and 64-bit
* `xmhf64 f41b83a6b`
* `xmhf64-nest 9840519b6`
* `lhv 87df18586`

## Behavior
As seen in `bug_085`, SMP Linux cannot boot in nested XMHF because INVEPT makes
handling interrupts so slow. Basically when Linux writes to LAPIC EOI, L2 XMHF
has to invalidate EPT, and L1 XMHF needs to reconstruct EPT by a number of
`L3 -> L1 -> L3` VMEXITs, which takes a long time.

So the goal of this bug is to make `KVM -> XMHF -> XMHF -> Debian` SMP
possible.

## Debugging

Possible solutions (mostly from `bug_085`):
* Support large pages in EPT
	* To implement this, need to first implement large pages in XMHF. This is
	  because that 
* Implement VMCS shadowing
	* This should definitely be implemented. It benefits a lot
* Use x2APIC
	* This can be considered a way to verify whether other aspects of Linux SMP
	  work
* Cache EPT violation history
	* This is likely a good solution for short / medium term
* For L2 XMHF, change EPT pointer instead of modifying EPT
	* I think it is hard to implement this solution cleanly, because to execute
	  the single instruction that accesses LAPIC, there are a lot of pages
	  that need to be mapped (at least RIP; what if the guest uses REP MOVS?)

### x2APIC

We first try booting Linux with x2APIC. To do this, simply compile and run
everything in 64-bit. Make sure `./build.sh` does not have `--no-x2apic`.
64-bit Linux will automatically use x2APIC.

However, looks like Linux needs NMI, so we get an error since NMI is not
implemented
```
0x0010:0xffffffff8566b3f4 -> (x2APIC ICR write) STARTUP IPI detected, EAX=0x0000069a, EDX=0x00000001
CPU(0x00): processSIPI, dest_lapic_id is 0x01
CPU(0x00): found AP to pass SIPI; id=0x01, vcpu=0x2025bfe0
CPU(0x00): destination CPU #0x01 has already received SIPI, ignoring
xmhf_smpguest_arch_x86vmx_eventhandler_x2apic_icrwrite: delinking LAPIC interception since all cores have SIPI
CPU(0x00): warning: cache miss
CPU(0x01): warning: cache miss
_vmx_handle_intercept_xsetbv: xcr_value=0x7
_vmx_handle_intercept_xsetbv: host cr4 value=0x00042020
_vmx_handle_intercept_xsetbv: xcr_value=0x7
_vmx_handle_intercept_xsetbv: host cr4 value=0x00042020
CPU(0x01): warning: cache miss
CPU(0x01): WRMSR (MTRR) 0x000002ff 0x0000000000000000 (old = 0x0000000000000c06)

Fatal: Halting! Condition '0 && "Nested guest NMI handling not implemented"' failed, line 624, file arch/x86/vmx/nested-x86vmx-handler.c
```

### NMI

We consider different cases of NMI configurations
* L0 is KVM (ignored)
* L1 is XMHF, always virtual NMIs = 1
* L2 is hypervisor, 3 cases
	* NMI exiting = 0
	* NMI exiting = 1
	* Virual NMIs = 1
* L3 guest

When L2 is running (VMX root), NMIs are handled by XMHF code. We only need to
consider the case when L3 is running (VMX non-root). `bug_018` section
`### Studying NMI` and `### NMI Write up` are likely helpful.

* NMI exiting = 0
	* Simply inject NMI to L3, return to L3 (invisible to L2)
	* L2 modifying "blocking by NMI" should result in L1 modifying the field
* NMI exiting = 1
	* Forward the NMI to L2, like other VMEXITs
	* L2 modifying "blocking by NMI" should result in L1 modifying the field
* Virual NMIs = 1
	* Forward the NMI to L2, like other VMEXITs
	* L2 owns the "blocking by NMI" VMCS field (indicating Virtual NMI
	  blocking) and "NMI-window exiting" VMCS field.

The challenging comes when writing XMHF's NMI exception handler. This handler
needs to deal with things asynchronously. We only need to handle the
two functions that may run with `vmx_nested_is_vmx_root_operation = 0`:
`_vmx_vmentry()` and `xmhf_nested_arch_x86vmx_handle_vmexit()`.
* For VMEXIT handler, just pretend that the NMI hits L2
* For VMENTRY handler, pretend that the NMI hits L2's VMLAUNCH / VMRESUME
  instruction

Maybe we should rewrite the NMI handlers to make things more clean. The normal
code can look like:
```
can_accept_nmi = 0;
critical_section;
can_accept_nmi = 1;
while (nmi_visited) {
	can_accept_nmi = 0;
	handle_nmi;
	nmi_visited = 1;
	can_accept_nmi = 1;
}
```

The NMI exception handler can look like:
```
if (can_accept_nmi) {
	handle_nmi;
} else {
	nmi_visited = 1;
}
```

Note that the above logic assumes that calling `handle_nmi` once or twice have
the same effect. This is true for the current logic.

The code is implemented in `xmhf64 1dde5ac37..39bfa074f`. Testing using Jenkins
looks fine. We also use `bug_018/qemu_inject_nmi.py` to test on QEMU.

To use `bug_018/qemu_inject_nmi.py`, add
`-qmp tcp:localhost:4444,server,nowait` to command line, and then
`python3 bug_018/qemu_inject_nmi.py 4444 0.1`

To make test more efficient, remove TrustVisor output in
`xmhf64-dev fd47fbe44`. However, when testing in this configuration, see that
NMI is likely injected into PALs. The following issue is observed with amd64
XMHF running `test_args32`, `test_args64` and `qemu_inject_nmi.py`, in 4 CPUs
KVM:
```
VMEXIT-EXCEPTION:
control_exception_bitmap=0xffffffff
interruption information=0x80000b0e
errorcode=0x00000000
	CPU(0x00): RFLAGS=0x0000000000010006
	SS:RSP =0x002b:0x00007ff76df70fc8
	CS:RIP =0x0033:0x00007ff76df9b0f3
	IDTR base:limit=0xfffffe0000000000:0x0fff
	GDTR base:limit=0xfffffe0000001000:0x007f
CPU(0x00): HALT; Nested events unhandled 0x80000202
```

This problem is also reproducible on i386 with otherwise same settings. But
looks like it takes a long time to reproduce. So need to be patient.
```
VMEXIT-EXCEPTION:
control_exception_bitmap=0xffffffff
interruption information=0x80000b0e
errorcode=0x00000000
	CPU(0x00): EFLAGS=0x00010092
	SS:ESP =0x007b:0xb7f2cfc4
	CS:EIP =0x0073:0xb7f5f01d
	IDTR base:limit=0xff400000:0x07ff
	GDTR base:limit=0xff401000:0x00ff
CPU(0x00): HALT; Nested events unhandled 0x80000202
```

Looks like `0xb7f5f01d` is at the start of a PAL.

This is also reproducible on i386 with one CPU and one `./test_args32` process.
It is likely that we can reproduce this bug by adding some HLT instructions
and inject NMI deterministically.

This is caused by incorrectly assuming that `xmhf_smpguest_arch_nmi_block()`
can write to hardware VMCS. However, actually this function is always called in
intercept handlers, and the hardware VMCS will always be overwritten. Fixed in
`xmhf64 3297ae84b`. So the final commits are `xmhf64 1dde5ac37..3297ae84b`

### PAL re-entry bug

While testing more on `xmhf64-dev 6d85380ee` in 64-bit KVM SMP=4, with 2
threads running `./test_args32 7 700000 7` and 1 thread running
`bug_018/inject_qemu_nmi.py 4444 1`, see the following bug after 117 NMIs.

Note that NMIs are injected very infrequently.

```
TV[3]:scode.c:hpt_scode_npf:1359:                  EU_CHK( ((whitelist[*curr].entry_v == rip) && (whitelist[*curr].entry_p == gpaddr))) failed
TV[3]:scode.c:hpt_scode_npf:1359:                  Invalid entry point
```

For this bug, CPU 0 produces the error. `scode_curr[*] = -1`,
`whitelist[1].gcr3 = vcpu->vmcs.guest_CR3`.
`vcpu->vmcs.guest_RIP = 0xf7fc108b`, which is body of `pal_10_int()`, but
`whitelist[1].entry_v = 0xf7fc101d`, which is start of `pal_10_int()`.
Similarly, `vcpu->vmcs.guest_paddr = 0xd50708b` and
`whitelist[1].entry_p = 0xd50701d`.

We need to reproduce this bug in different settings. I guess this may be
unrelated to NMI. Unfortunately, this bug is very hard to reproduce, even with
the same configurations. Also, we should prioritize correctness on real
hardware. So skip for now.

### Virtualize NMIs

Let's think about a story of L1 guest hypervisor running L2, where L0 is XMHF.
1. L1 calls VMENTRY (VMLAUNCH or VMRESUME), trap to L0
2. L0 converts VMCS12 to VMCS02, enter L2
3. L2 runs, page fault
4. L0 handles page fault, go back to L2
5. L2 runs, CPUID
6. L0 converts VMCS02 to VMCS12, enter L1
7. L1 handles L2's VMEXIT
8. ..., L2 runs again
9. L2 gets NMI, cause NMI VMEXIT (not due to quiesce)

I thought about the previous comment that "For VMENTRY handler, pretend that
the NMI hits L2's VMLAUNCH / VMRESUME instruction". Now I think that this is a
bad idea because it is hard for L0's NMI interrupt handler to distinguish
between step 2 and step 6.

The most of L0 should be NMI disabled using the
`xmhf_smpguest_arch_x86vmx_mhv_nmi_disable()` function we just wrote. However,
we need to add logic during context switch between serving L1 and L2.

Still, enumerate all cases using `bug_018` section `### NMI Write up`

* NMI Exiting = 0, virtual NMIs = 0
	* During VMCS translation, copy "Blocking by NMI" and "NMI-window exiting"
	  from VMCS01 to VMCS02
	* `vcpu->vmx_guest_nmi_cfg` is shared, no need to copy, though
	* Step 2: if NMI visited L0, set "NMI-window exiting" bit of L2. Should
	  be able to reuse `xmhf_smpguest_arch_x86vmx_inject_nmi()`.
	* Step 4: same as step 2
	* Step 6: similar to step 2 and to normal L1 handling
* NMI Exiting = 1, virtual NMIs = 0
	* L1 owns "Blocking by NMI" and "NMI-window exiting" in VMCS12, copy to
	  VMCS02 (except when needing L2 to run a few instructions).
	* `vcpu->vmx_guest_nmi_cfg` is ignored in some sense when L0 is running L2.
	* Step 2: if NMI visits L0, convert VMCS02 back to VMCS12 and NMI exit.
	  May need to let L2 run a few instructions when VMCS02 contains an event
	  injection (need experiment)
	* Step 4: same as step 2
	* Step 6: go to normal L1 handling (i.e. inject NMI to L1)
* NMI Exiting = 1, virtual NMIs = 1
	* Same as NMI Exiting = 1, virtual NMIs = 0

We need to experimentally test how different NMI configurations behave, because
the SDM is not very straight forward. In the best case, the experiments should
be automated.

* NMI Exiting = 0, virtual NMIs = 0: when L2 does not block NMI. If L2 receives
  an NMI, it invokes L2's exception handler. `experiment_1()`: correct
* NMI Exiting = 0, virtual NMIs = 0: when L1 blocks NMI, L2 does not block. If
  L1 receives an NMI, then VMENTRY to L2, will L2 get the NMI?
  `experiment_3()`: yes
* NMI Exiting = 0, virtual NMIs = 0: when L1 does not block NMI, set VMCS so
  that L2 blocks. After L2 VMEXIT to L1, will L1 also block NMI?
  `experiment_4()`: yes
* NMI Exiting = 0, virtual NMIs = 0: when L1 does not block NMI, L2 blocks. If
  L2 receives an NMI, then VMEXIT to L1 (L1 may need to IRET), will L1 get the
  NMI? `experiment_2()`: no
* NMI Exiting = 1, virtual NMIs = 0: when L2 does not block NMI, if L2 receives
  NMI, VMEXIT happens. `experiment_5()`: correct
* NMI Exiting = 1, virtual NMIs = 0: when L2 blocks NMI, if L2 receives NMI,
  VMEXIT happens. `experiment_6()`: correct
* NMI Exiting = 1, virtual NMIs = 0: when L1 blocks NMI, L2 any configuration.
  If L1 receives an NMI, then VMENTRY to L2, will L2 VMEXIT to L1?
  `experiment_7()`: yes
* ...

### Possible KVM bug

While writing tests, found a possible KVM bug. Reproduce in
`lhv-dev 82773f901`. To reproduce this bug, set VMCS to NMI Exiting = 0,
virtual NMIs = 0, Blocking by NMI = 1. Then let L2 halt using HLT and send only
NMIs to L2. KVM shows that L2 will be interrupted by NMI. However, Bochs shows
that L2 will never be interrupted.
* Note: later, it looks like L2 NMI handler is not raised, but CPU resumes from
  HLT instruction.

I am worried that KVM cannot be relied on for this purpose. Bochs should be
used instead. Should run Bochs on newer machines to make it fast.

Then when writing `lhv-dev 8c235cd81`, I realized that QEMU does not raise any
interrupt handler. Instead, it just resumes the HLT handler. So to workaround
this QEMU problem, we can simply make HLT a conditional loop. See
`lhv-dev 8fd86d991`.

Then, I realized that Bochs may also have bugs. In `lhv-dev 35f7d6fb9`,
`experiment_3()` fails on Bochs, but succeeds on QEMU and Thinkpad. So from now
on we need to be careful when testing on both QEMU and Bochs.

KVM / Bochs bugs
* Make sure all experiments can run
	* Some experiments in Bochs are not stable
* When NMI hits HLT instruction, VMCS is different from Thinkpad
	* Common RIP: immediately after the HLT instruction
	* Thinkpad: Activity state = 1, VMEXIT inst length = 1
	* KVM: Activity state = 0, VMEXIT inst length = 1
	* Bochs: Activity state = 0, VMEXIT inst length = 0
	* To reproduce, run `experiment_5()` and print VMCS in NMI VMEXIT handler.

### Conclusion of experiments 1 - 13

After implementing 13 experiments in `lhv-dev aa8d89b1c..254e7d222`, we
summarize the VMX behavior related to NMI below. Note that looks like there are
a lot of bugs in KVM and Bochs. So KVM and Bochs can only be used to debug the
experiment code. The results on real hardware is the ground truth.

* Non VMX related
	* In host, IRET removes NMI blocking
	* If NMI pending and host does not block NMI, host NMI interrupt handler is
	  executed
* NMI Exiting = 0, virtual NMIs = 0
	* When VMENTRY, guest's NMI blocking status is determined by VMCS
	* When non-NMI VMEXIT, host's NMI blocking is determined by guest's NMI
	  blocking status (i.e. does not change)
	* In guest, IRET removes NMI blocking
	* If NMI pending and guest does not block NMI, guest NMI interrupt handler
	  is executed
* NMI Exiting = 1, virtual NMIs = 0
	* When VMENTRY, guest's NMI blocking status is determined by VMCS
	* When non-NMI VMEXIT, host's NMI blocking is determined by guest's NMI
	  blocking status (i.e. does not change)
	* In guest, IRET does not change NMI blocking status
	* If NMI pending and guest does not block NMI, NMI VMEXIT to host
* NMI Exiting = 1, virtual NMIs = 1
	* When VMENTRY, guest's virtual NMI blocking status is determined by VMCS
	* When VMENTRY, guest's NMI blocking status is (does not matter)
	* When non-NMI VMEXIT, host's NMI blocking status is not blocking
	* In guest, IRET changes virtual NMI blocking status
	* If NMI pending and always, NMI VMEXIT to host

We can rephrase the above:
* When VMENTRY,
	* Guest NMI blocking is
		* If virtual NMIs = 0, determined by VMCS
		* If virtual NMIs = 1, not blocking
	* Guest virtual NMI blocking is
		* (Assert virtual NMIs = 1), determined by VMCS
* When non-NMI VMEXIT,
	* Host NMI blocking is
		* The same as guest NMI blocking (i.e. does not change)
* When guest executes IRET
	* If NMI Exiting = 0, virtual NMIs = 0, remove NMI blocking
	* If NMI Exiting = 1, virtual NMIs = 0, no effect
	* If NMI Exiting = 1, virtual NMIs = 1, remove virtual NMI blocking
* When NMI pending and guest does not block NMI,
	* If NMI Exiting = 0, execute guest NMI interrupt handler
	* If NMI Exiting = 1, VMEXIT

For now, we can start writing XMHF code. In the future, may need to add some
experiments for NMI window exiting.

We need to design how NMI blocking are tracked
* Host NMI blocking is always tracked in VMCS01's guest interruptibility
* Guest NMI blocking
	* If NMI Exiting = 0, virtual NMIs = 0, use VMCS02's guest interruptibility
	* If NMI Exiting = 1, virtual NMIs = 0, use a new variable (copy from host)
	* If NMI Exiting = 1, virtual NMIs = 1, use a new variable (set to 0)
* Guest virtual NMI blocking: use VMCS02's guest interruptibility
* L0 injecting NMI to L2
	* If NMI Exiting = 0, use NMI window exiting bit in VMCS02
	* If NMI Exiting = 1, manually check the "new variable" above

In `xmhf64-nest 044fa9b95`, implemented logic that passes the first 4
experiments, which are all experiments for NMI Exiting = 0, virtual NMIs = 0.
Testing on QEMU looks good.

In `xmhf64-nest-dev 6b6187186..47f04cc0f`, able to pass experiment 1 - 5.
However, I think the way to pass experiment 5 is not clean. So not committing
to `xmhf64-nest` for now.

The alternative implementation is in `xmhf64-nest 40ae1ef3b..03cb5dd3e`. In
this implementation, NMI windowing exits prefix all NMI VMEXITs. Though it may
be slow, this makes sure that NMI VMEXITs will not overwrite interruption
information / IDT vectoring information etc. This implementation passes
all existing experiment (1 - 14).

### New experiments

* Experiment 14 - 17: L1 block NMI, receive NMI, VMENTRY to L2 (not blocking
  NMI) but also inject a keyboard interrupt. Result: The injected interrupt
  comes first before NMI guest interrupt / NMI VMEXIT.
* Experiment 18 - 20: L1 inject NMI to L2, observe whether L2 blocks NMI.
  Result: NMIs are not blocked; virtual NMIs (when available) are blocked.
	* Looks like Bochs and QEMU have a bug on this. We may end up also need to
	  have this bug.

The above experiments are implemented in `lhv-dev a770d519a..f3eef2219`.
Thinkpad passes all the experiments. As predicted, `xmhf64-nest cc49ab14a`
passes all except 18. For now, we let XMHF panic when experiment 18 is
detected. Implemented in `xmhf64-nest cc49ab14a..ab678ccba`.

* Experiment 21 - 23: Test normal behavior of NMI windowing. These are straight
  forward after reading the SDM. These experiments work on all platforms. These
  tests are passed in XMHF.
* Experiment 24 - 26: Test priority between NMI windowing, NMI injection to
  guest, and NMI exiting. The conclusion is that
  `NMI injection > NMI windowing > NMI exiting`.

XMHF on KVM fails experiment 24 and 26. For 24, KVM also fails the experiment.
So I guess XMHF will pass when running on real hardware, so now worrying for
now. For 26, XMHF's priority is that `NMI exiting > NMI windowing`.

Experiment 26 is a limitation of current implementation of XMHF, similar to
experiment 18. To solve this problem, we cannot rely on NMI windowing to
implement "inject NMI to guest and VMEXIT immediately".

One possible way is to implement using monitor trap. However, at least Circle
CI's KVM does not support monitor trap.

Another possible way is to use the `#DB` exception, as used in APIC
virtualization. However, this solution is likely not going to work, because
`#DB` requires changing EFLAGS.TF, but exception injection involves pushing
EFLAGS to the stack.

The final solution is to emulate the event injection. However, maybe we can
live without the exact behavior as hardware.

### Testing on KVM XMHF XMHF Debianx64

As a review, the command we use to compile and run XMHF in this configuration
are:

```sh
./build.sh amd64 fast --sl-base 0x20000000 circleci --no-init-smp && gr
./build.sh amd64 fast circleci && gr
(cd build32 && make -j 4 && gr) && (cd build64 && make -j 4 && gr) && tmp/bios-qemu.sh -smp 2 -m 1G --gdb 2198 -d build32 +1 -d build64 +1 -d debian11x64 +1
```

Need to modify XMHF so that it does not print things when nested VMEXIT /
VMENTRY happens. Also, change `EPT02_PAGE_POOL_SIZE` from 128 to 192. Then
KVM XMHF XMHF Debian x64 boots. When executing `./test_args32 7 7 7`, see the
following exception

```
Fatal: Halting! Condition 'vmcs12->control_VM_entry_exception_errorcode == __vmx_vmread32(0x4018)' failed, line 443, file arch/x86/vmx/nested-x86vmx-vmcs12-fields.h
```

This basically means `control_VM_entry_exception_errorcode` cannot be
`FIELD_PROP_SWWRONLY`, because it is used in functions such as
`xmhf_nested_arch_x86vmx_handle_vmexit()` to inject NMIs.

Now we review VMCS02 fields that are changed due to NMI handling
* `guest_interruptibility`: NMI blocking
* `control_VMX_pin_based`: NMI 
* `control_VMX_cpu_based`: NMI windowing
* `control_VM_entry_interruption_information`: inject NMI
* `control_VM_entry_exception_errorcode`: inject NMI

Looks like the first 3 are handled well in `nested-x86vmx-vmcs12.c`. However,
the latter 2 are incorrect. Actually, this problem also happens in EPT
handling, but does not trigger a bug.

The problem is that VMCS02 will change
`control_VM_entry_interruption_information` and
`control_VM_entry_exception_errorcode` to virtualize EPT or NMI. However, this
change should be invisible from VMCS12. We should consult the manual to see
the correct behavior.

The correct behavior is simple. In Intel v3
"23.8.3 VM-Entry Controls for Event Injection":
> VM exits clear the valid bit (bit 31) in the VM-entry
> interruption-information field.

So the correct behavior is to clear that bit in VMCS12, and ignore whether
VMCS02 and VMCS12 match. Implemented in `xmhf64-nest bd7c46c83`. After the
change, running KVM XMHF XMHF Debian x64 looks good.

In `xmhf64-nest 65da0f4d8`, add check to warn when a situation similar to
experiment 26 happens.

### Extending supported configurations

Amd64 XMHF looks fine when passing the tests.

However, when compiled with circleci (`--enable-optimize-nested-virt`),
multiple tests fail, like experiment 4. This is likely because
`_optimize_x86vmx_intercept_handler()` does not handle things like
`xmhf_smpguest_arch_x86vmx_mhv_nmi_disable()`. This should be considered a bug.
It is fixed in `xmhf64 330e650f4` and `xmhf64-nest 00713eda7`.

Actually the problem may be different. When `case VMX_VMEXIT_VMWRITE` in
`_optimize_x86vmx_intercept_handler()` is commented out, the problem
disappears. We brute force to find which VMCS field is not read and causes the
problem. See `409ca2cd7` in `xmhf64-nest-dev`.

After some bisecting, looks like
`vcpu->vmcs.control_VM_entry_interruption_information` needs to be read.
Currently it is only written. This problem is strange.

For now, abbreviate `control_VM_entry_interruption_information` as CVEII.

After some print debugging, looks like the story is
* During some previous VMEXIT, an NMI is injected, so
  `VMCS.CVEII = vcpu->vmcs.CVEII = 0x80000202`, and VMENTRY to L1.
* During the next VMWRITE VMEXIT, `VMCS.CVEII` is cleared by hardware, but
  `vcpu->vmcs.CVEII` is not. So `VMCS.CVEII = 0x00000202`, and
  `vcpu->vmcs.CVEII = 0x80000202`
* After VMWRITE VMEXIT is processed, `vcpu->vmcs.CVEII` is written to
  `VMCS.CVEII`. So an extra NMI is injected.

This bug is fixed in `xmhf64-nest 8044db960`. After this bug is fixed, looks
like NMI experiments can pass with `--enable-optimize-nested-virt`. Both 32 bit
and 64 bit work fine.

### Experiment 24

Basically experiment 24 fails on a variety of configurations
* KVM LHV
* KVM XMHF LHV
* XMHF LHV

The behavior are basically the same:

```
Experiment: 24
CPU(0x01): nested vmexit 18
  Enter host, exp=24, state=0
    hlt_wait() begin, source =  EXIT_NMI_H   (5)
      Inject NMI
CPU(0x01): inject NMI
      Interrupt recorded:       EXIT_NMI_H   (5)
    hlt_wait() end
    hlt_wait() begin, source =  EXIT_TIMER_H (6)
      Inject interrupt
      Interrupt recorded:       EXIT_TIMER_H (6)
    hlt_wait() end
    hlt_wait() begin, source =  EXIT_TIMER_H (6)
      Inject NMI
      Strange wakeup from HLT
      Inject interrupt
      Interrupt recorded:       EXIT_TIMER_H (6)
    hlt_wait() end
  Leave host
CPU(0x01): nested vmentry
CPU(0x01): nested vmexit 0
      Interrupt 1 recorded:     EXIT_VMEXIT  (7)
CPU(0x01): nested vmentry
CPU(0x01): nested vmexit 8
      Interrupt recorded:       EXIT_NMIWIND (8)
CPU(0x01): nested vmentry

source1:         EXIT_NMI_H   (5)
exit_source_old: EXIT_VMEXIT  (7)
TEST_ASSERT '0 && (exit_source_old == source1)' failed, line 378, file lhv-guest.c
```

The expected behavior is that NMI windowing happens first, then NMI hits the
host. However, the actual behavior is that VMEXIT happens first, after that
entering the guest triggers NMI window.

The priority of NMI window and NMI VMEXIT are swapped in function
`xmhf_nested_arch_x86vmx_handle_vmexit()` in `xmhf64-nest 92bb54710`. Now
XMHF passes experiment 24 (even on QEMU).

Again, XMHF does not pass experiment 18 and 26 due to technical challenges.
Currently there are code to detect these cases and halt / warn the user. We
are not going to worry about these in the short term.

At this point, XMHF on real hardware also passes all the tests. Tested 32 bit
and 64 bit XMHF. "Strange wakeup from HLT" does not show up.

However, running Thinkpad XMHF XMHF Fedora results in error. Looks like APs
will at some point receive two INIT signals, which cause them to execute
shutdown code. Not going to debug it at this point, since debugging is
inconvenient.

To run XMHF on Thinkpad, a few things need to be taken care of
* Due to e820 mapping, use `--with-sl-base=0x20200000` instead of `0x20000000`
* Increase ucode update macro: `#define UCODE_TOTAL_SIZE_MAX (4 * PAGE_SIZE_4K)`

### Testing NMI dropping

In `lhv-dev 3cee6b73a`, implemented experiment 27 to test NMI dropping when too
many NMIs are received by the CPU. However, I feel that Thinkpad's behavior is
incorrect. Not going to worry about it for now.

A new branch, `lhv-nmi`, is created for NMI testing code. `lhv-dev` will be
reverted to be the same as `lhv`.

Future TODOs
* Pass experiments 18, 26 on XMHF
* Pass experiment 27 on Thinkpad
* Write more tests on NMI blocking (similar to experiment 27)
* Report bugs to KVM and Bochs

## Fix

`xmhf64 f41b83a6b..330e650f4`
* Simplify NMI handling logic by delaying processing of NMIs

`xmhf64-nest 9840519b6..84b01ca77`
* Implement NMI virtualization
* Add logic to allow detecting XMHF through CPUID

`lhv-nmi aa8d89b1c..3cee6b73a`
* Implement tests on NMI behavior in VMX

