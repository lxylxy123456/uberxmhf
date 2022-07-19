# Running SMP Linux in XMHF in XMHF

## Scope
* KVM
* 32-bit and 64-bit
* `xmhf64 f41b83a6b`
* `xmhf64-nest 9840519b6`

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
unrelated to NMI.

TODO: try to reproduce the bug above

