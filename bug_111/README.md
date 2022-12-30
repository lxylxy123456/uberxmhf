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

### XMHF

TODO

