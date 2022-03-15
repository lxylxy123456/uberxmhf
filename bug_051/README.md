# Mask NMIs when running PALs

## Scope
* All configurations
* Directly related to TrustVisor only
* Git `65a3f5ea4`

## Behavior
Currently nothing stops NMI from being delivered while a guest is running a
PAL. On serial it will show "drop NMI". Normally NMI should result in
"inject NMI". TrustVisor should be modified to block NMI in PALs and unblock
afterwards.

## Debugging

### Maskable interrupts
In scode.c, maskable interrupts are masked
* In `hpt_scode_switch_scode()`, disable interrupts:
  `VCPU_grflags_set(vcpu, VCPU_grflags(vcpu) & ~EFLAGS_IF);`
* In `hpt_scode_switch_regular()`, enable interrupts:
  `VCPU_grflags_set(vcpu, VCPU_grflags(vcpu) | EFLAGS_IF);`

### NMI
Virtual NMI blocking can be controlled using "Interruptibility State" bit 3.
Documentation in Intel v3 "23.4.2 Guest Non-Register State" p903
"Table 23-3. Format of Interruptibility State".

In XMHF this is `DECLARE_FIELD(0x4824, guest_interruptibility)`

### Testing

Currently git at `fbabf202a`, need to test.

In `xmhf64-dev` branch, `f351d1441` tests TrustVisor after change. Use QEMU to
inject NMI. Use `./main64 3 30000000` to sleep for a long time.

Can see that when PAL is sleeping, NMI does not come up. When PAL
ends, the screen prints "CPU(0x00): inject NMI".

`8762a4958` tests TrustVisor before change. Sometimes can see
"CPU(0x00): drop NMI".

So this test shows that the changes are good.

## Fix

`65a3f5ea4..f0baec0bb`
* Check assumptions about EFLAGS.IF in TrustVisor
* Block NMI when running PALs
* Halt if dropping NMI

