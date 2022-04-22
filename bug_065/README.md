# Use APIC virtualization

## Scope
* All configurations
* Git `016013a8c`

## Behavior

Currently when guest accesses APIC (e.g. waking up other CPUs), two intercepts
are made, using not-so-clean techniques. This may be inefficient and cause
problems in extreme cases.

Intel i3 shows that there is a feature called APIC virtualization. Maybe we
should use it. See Chapter 28: "APIC VIRTUALIZATION AND VIRTUAL INTERRUPTS"

## Debugging

First we analyze what the code is currently doing.

`peh-x86vmx-main.c` handles all interceptions (VMEXITs). It calls handlers in
`smpg-x86vmx.c` to process

* `xmhf_smpguest_arch_x86vmx_eventhandler_hwpgtblviolation()`
	* Called when guest accesses APIC page
	* Records the guest's access type (read / write, address)
	* Temporarily change APIC page mapping in EPT
	* Use `#DB` and disable interrupts to set up single step
* `xmhf_smpguest_arch_x86vmx_eventhandler_dbexception()`
	* For INIT IPI and STARTUP IPI (SIPI), handle them virtually. For anything
	  else, propagate to physical LAPIC
	* Clean up single step and paging

The current solution has some problems
* Code is huge (bad for security & verification)
* Many cases that should be fast end up being 2 intercepts (slow)
	* For example, `### LAPIC` in `bug_064` can be avoided. In `bug_064` the
	  CPU is writing to the EOI register (0xb0), but XMHF is only interested in
	  the ICR registers (0x300 and 0x310).
* Using `#DB` and disable interrupts to set up single step is wrong
	* Should use monotor trap
	* Even maskable interrupts are disabled, what about NMI?
* APs (CPU other than BSP) do not have these functions? So what will happen if
  the OS uses APs to wake up other processors (for example proposed in
  <https://stackoverflow.com/a/70148739>, see `My Method`)
* x2APIC cannot be supported easily

We need to study Intel's documentation

### Studying APIC virtualization

"28.4.3.2 APIC-Write Emulation" says, when accessing ICR low,
> If the "virtual-interrupt delivery" VM-execution control is 0, or ..., the
> processor causes an APIC-write VM exit (Section 28.4.3.3).

We should not set the "virtual-interrupt delivery" flag.

However, the bad news is that "APIC-Access VM Exits" and "APIC-Write VM Exits"
also do not indicate the value written. So maybe we should stick to EPT.
However, we can consider fixing the single step problem, and support x2APIC.

### Studying single step

From "24.5.2 Monitor Trap Flag", it is possible to inject a "pending MTF VM
exit", which may fit our needs. Otherwise, just setting the monitor trap flag
is good.

We should try injecting MTF first.

In git `cdb69a3b7`, try to inject MTF. However, looks like nothing happened.
So we try to set monitor trap flag instead.

Things to do for LAPIC

* Replace `#DB` with monitor trap
* Disable NMI
* Capture exceptions

Using monitor trap flag seems working. See git `336240b04`. So we will use it.

### Fixing `xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception()`

`xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception()` assumes that
`vcpu->vmcs.control_VMX_cpu_based` is read-only, otherwise there will be
race conditions. We change the logic to remove this problem. Reasoning is
clearly documented in that function, see commit `617b027c8`.

### Replacing `#DB`

Git `f5c27d852` replaces `#DB` with monitor trap. Looks like AMD does not
support monitor trap, so AMD still needs `#DB`. So we do not change related
function names.

Possible clues about AMD's support status on monitor trap
* <https://lore.kernel.org/all/20220128005208.4008533-23-seanjc@google.com/>:
  `/* SVM has no hyperversior debug trap (VMX's Monitor Trap Flag). */`
* See also: <https://stackoverflow.com/q/36480612>

### Disable NMI, capture exceptions

Git `ed054072d` disables NMI during LAPIC interception handling.

I also realized that the testing script for Jenkins can be used to test
automatically.
```py
python3 ../xmhf64/.jenkins/test2.py --subarch amd64 --xmhf-bin . --qemu-image ../tmp/qemu/debian11x64-t.qcow2 --qemu-image-back ../tmp/qemu/debian11x64.qcow2 --smp 4 --work-dir /tmp/t
```

Git `ae0d0ffea` captures exceptions.

### x2APIC

Git `876b85972..e9a928acf` tries to add x2APIC support. However, looks like
after x2APIC is used. LAPIC no longer works, so hypervisor cannot access LAPIC
(when using NMI to quiesce other CPUs).

This is covered in "10.12.5 x2APIC State Transitions". Basically need to look
at the `IA32_APIC_BASE` register's bit 10 ("Enable x2APIC mode.")

Git `876b85972..d134d2d84` contains more changes in the `xmhf64-dev` branch.
They are squashed to commit `a13c6bfe1` in `xmhf64` branch.

Looks like only x64 Debian in QEMU uses x2APIC. x86 Debian does not use x2APIC.
HP does not support x2APIC.

### Circle CI regression

A regression happens in `617b027c8..ac146f45b`. After EPT violation
interception handler, VMRESUME fails.

The problem happens after the first EPT. We can compare the entire VMCS before
and after

```diff
46c46
< CPU(0x00): vcpu->vmcs.control_VMX_cpu_based=0x86006172
---
> CPU(0x00): vcpu->vmcs.control_VMX_cpu_based=0x8e006172
48c48
< CPU(0x00): vcpu->vmcs.control_exception_bitmap=0x00000000
---
> CPU(0x00): vcpu->vmcs.control_exception_bitmap=0xffffffff
132c132
< CPU(0x00): vcpu->vmcs.guest_interruptibility=0x00000000
---
> CPU(0x00): vcpu->vmcs.guest_interruptibility=0x00000008
150c150
< CPU(0x00): vcpu->vmcs.info_vminstr_error=0x00000000
---
> CPU(0x00): vcpu->vmcs.info_vminstr_error=0x00000007
```

My guess is that the host does not support monitor trap. The exception bitmap
and interruptibility should be supported (used in other places).

`vcpu->vmcs.control_VMX_cpu_based` is called 
"Primary Processor-Based VM-Execution Controls". Should consule
`IA32_VMX_PROCBASED_CTLS` (0x482). The value is
`vcpu->vmx_msrs[2]=0xf7b9fffe0401e172`.

We can see that monitor trap is indeed not supported. We should fall back to
`#DB`.

### Fall back to `#DB`

The commit changing `#DB` to monitor trap is `f5c27d852`

### Still Circle CI regression

At `02ab74d97`, amd64 regression is fixed by optimizing EOI in x2APIC. However,
i386 fails (stuck at a strange place).

We may need to bisect

* `be45d427f` is bad (`xmhf64-dev`)
* `02ab74d97` is bad (`xmhf64`) (amd64 is good)
* `dea6db4ea` is bad
* `617b027c8` is good

The problem is that commits in between starts with monitor trap, which makes
everything bad. So we need to do manual work.

We start a new branch `xmhf64-bug-065` from `617b027c8`, and move towards
`dea6db4ea`.

Then found that this is a bug caused by carelessness. The `xmhf64-bug-065`
branch ends at `f8d51ce4c`.

## Fix

`5c29efd57..3d1952ab3`
* Update NMI quiesce code
* Support x2APIC

