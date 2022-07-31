# Running SMP Linux in XMHF in XMHF

## Scope
* KVM
* 32-bit and 64-bit
* No x2APIC
* `xmhf64 330e650f4`
* `xmhf64-nest 84b01ca77`
* `lhv 375bbf44f`

## Behavior
When running KVM XMHF XMHF Debian, booting is too slow because Debian accesses
LAPIC, which causes EPT violations in L2 XMHF. L2 XMHF executes INVEPT, which
flushes all mappings in L1 XMHF. This becomes too slow for Debian to boot. Some
optimizations are needed. See `bug_085` for related information.

## Debugging

From `bug_085`, we can try:
* Implement VMCS shadowing
* Support large pages in EPT
* Cache historical EPT violation addresses

### Using VMCS shadowing

At least KVM supports VMCS shadowing. So if L1 XMHF uses VMCS shadowing, L1 can
save time processing VMREAD and VMWRITE from L2 hypervisor.

TODO: set VMREAD-bitmap and VMWRITE-bitmap to 0s
TODO: fix xmhf64-nest CI
TODO: implement VMPTRLD and VMXOFF
TODO: test VMCLEAR (e.g. VMCLEAR then VMREAD)

