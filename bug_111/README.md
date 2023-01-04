# Implement Instruction Emulation

## Scope
* All configurations
* `xmhf64 e641acea5`
* `xmhf64-dev 1cd113b58`
* `lhv 37f8ad0bd`
* `lhv-dev b10f8c919`

## Behavior

Currently XMHF modifies EPT and uses instruction stepping to simulate
instruction emulation on LAPIC memory. However, this solution is not elegant.
It is better to really read the instruction and emulation it.

## Debugging

### GPL

An example is instruction emulator in Xen. Looks like KVM code also uses Xen's
emulator in `arch/x86/kvm/emulate.c`. However, the code is marked as
"SPDX-License-Identifier: GPL-2.0-only", but XMHF code is BSD-3-clause. There
is a licence conflict.

### KVM Emulation of LOCK prefix

I wonder how KVM implements the LOCK prefix. It appears there are no locks
used to synchronize.

Looks like KVM may be implementing something similar to
Load-Linked / Store Conditional (LL/SC). The call stack is something like:

```
x86_emulate_insn
	writeback
		segmented_cmpxchg
			emulator_cmpxchg_emulated (ctxt->ops->cmpxchg_emulated)
				CMPXCHG64
					CMPXCHG instruction
				Return X86EMUL_CMPXCHG_FAILED if fail
```

However, does `x86_emulate_insn` correctly retry if see
`X86EMUL_CMPXCHG_FAILED`? Looks like it just returns `EMULATION_OK`

Try this:
* Hopefully 0xb8000 memory write is emulated
* CPU 0: loop of: write 1 to 0xb8000, increase counter
* CPU 1 and CPU 2: loop of: XCHG(*0xb8000, 0), increase counter if see 1
* Should see `counter_0 = counter_1 + counter_2`
* If QEMU is wrong, may see `counter_0 < counter_1 + counter_2`

This bug is reproduced in `lhv-dev dba0a2e7f`. TCG does not have this bug. Real
hardware does not have this bug.

The bug is reported in <https://bugzilla.kernel.org/show_bug.cgi?id=216867>.
Will track status in `bug_076`. Bug report text in `kvm.txt`, image in
`c.img.gz`.

Also tested this on other hypervisors
* Virtual Box, Linux host: good
* VMware, Linux host: bad (similar to KVM, but takes longer to deadlock)
* VMware, Windows host with Hyper-V: bad (same as VMware Linux host)
* Hyper-V, Windows host: good (hard to use serial port, so use VGA, see
  `lhv-dev 46e4c60af`)

#### VMware bug report

The guideline to report defect is in:
<https://www.vmware.com/support/policies/defect.html>. I don't think I have an
active support agreement, so reporting to VMware Technology Network.

Create the VMDK file using:

```sh
xz -dk c.img.xz
qemu-img convert -f raw -O vmdk c.img c.vmdk
```

The bug report text is in `vmware.txt`. Bug reported in:
<https://communities.vmware.com/t5/VMware-Workstation-Player/LOCK-instruction-atomicity-broken-on-VGA-memory-mapped-IO/m-p/2946505#M39982>

#### Unaligned memory access

It becomes challenging to support LOCK CMPXCHG8B. There are no alignment
requirements for LOCK and CMPXCHG8B (there is for CMPXCHG16B, though). So it
is possible for the memory access to cross page boundary.

We can make 2 physical pages form a ring in virtual address:
* PA=0xaaaa0000, VA=0xbbbb0000
* PA=0xaaaa1000, VA=0xbbbb1000
* PA=0xaaaa0000, VA=0xbbbb2000

Then we can access each 4 byte aligned address, but access size is 8 bytes:
* VA=0xbbbb0000, 0xbbbb0004, 0xbbbb0008, ..., 0xbbbb0ff8
* VA=0xbbbb0ffc: this access crosses page boundary
* VA=0xbbbb1000, 0xbbbb1004, 0xbbbb1008, ..., 0xbbbb1ff8
* VA=0xbbbb1ffc: this access crosses page boundary

To implement this correctly, the CMPXCHG instruction in KVM needs to access 2
pages at once. So KVM needs to be able to map these 2 pages consecutively in
both orders. This becomes a challenge for XMHF because all guest memory are
identity mapped.

Looks like KVM does not emulate this case at all. KVM simply implements the
access as normal memory write. See `emulator_cmpxchg_emulated()`:

```c
6429  	/*
6430  	 * Emulate the atomic as a straight write to avoid #AC if SLD is
6431  	 * enabled in the host and the access splits a cache line.
6432  	 */
6433  	if (boot_cpu_has(X86_FEATURE_SPLIT_LOCK_DETECT))
6434  		page_line_mask = ~(cache_line_size() - 1);
6435  	else
6436  		page_line_mask = PAGE_MASK;
6437  
6438  	if (((gpa + bytes - 1) & page_line_mask) != (gpa & page_line_mask))
6439  		goto emul_write;
```

### XMHF

In XMHF, we are not expecting to implement as many instructions as in Xen.
The information flow can be represented as:
* `peh -> emulator`: please emulate ADDL instruction
* `emulator -> memory`: please read address 0x12345678
* `memory -> emulator`: read data 0x0
* `emulator -> memory`: please write 0x1 to 0x12345678
* `memory -> emulator`: written

In KVM there are function pointers for the emulator to communicate to other
components, such as `ctxt->ops->cmpxchg_emulated`. Here I am planning to use
normal function calls. It should be easy to replace the function calls with
function pointer calls later.

In `xmhf64 839937397..d11e8b841`, check segment limit and permissions in
`_vmx_decode_seg()`.

In `xmhf64-dev 1cd113b58..c90491fdd`, implement instruction emulation for some
MOV instructions. 32-bit LHV can run now, 64-bit cannot run yet because REX
prefix is not implemented yet. x86 Debian stucks with CPU 0 ISR = 253.

In `xmhf64-dev 3e8d5df89`, able to emulate for 64-bit guests. x64 Debian with
x2APIC works correctly. If XMHF hides x2APIC, x64 Debian also stucks with CPU 0
ISR = 253. Looks like this is the known KVM bug reported in
<https://bugzilla.kernel.org/show_bug.cgi?id=216045>. The workaround for this
case is to add `-machine kernel_irqchip=off`. After that Debian x86 and x64 can
boot correctly.

This may become a good way to debug the KVM bug. I am pretty sure that using
`#DB` exception is good, but using instruction emulation is bad.

In `xmhf64-dev 1be476896`, support more Windows guests, but unstable. Giving up
now because it is too difficult to write instruction emulation cleanly.
* To see the change, use `git diff 4cfa1eee1 1be476896`.

## Fix

`xmhf64 e641acea5..4cfa1eee1`
* Add segmentation limit and access rights check to `guestmem_desegment()`

`xmhf64-dev 1cd113b58..a864b512e`
* Implement some of instruction emulation, then reverted

