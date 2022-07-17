# Running SMP Linux in XMHF in XMHF

## Scope
* KVM
* 32-bit and 64-bit
* `xmhf64-nest b6e3637b4`

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
* Implement VMCS shadowing
* Use x2APIC
* Cache EPT violation history
* For L2 XMHF, change EPT pointer instead of modifying EPT

