# Support Double INIT-SIPI-SIPI (for UEFI SMP on KVM)

## Scope
* KVM (not on Dell 7050)
* UEFI (not on BIOS)
* `xmhf64 a7fb6764b`
* `xmhf64-dev b7411a807`

## Behavior

From `bug_071`, see section `### Redesign INIT intercept handler`

When booting UEFI SMP, the guest performs INIT-SIPI-SIPI twice. This is not
supported by XMHF, so XMHF thinks the guest is trying to reset.

## Debugging

In `xmhf64-dev cb0c064d6`, implement INIT-SIPI-SIPI twice. During compile time
the programmer need to specify number of times the guest performs
INIT-SIPI-SIPI (this prevents AP from sending message to BSP to change BSP's
EPT).

The full implementation is in `xmhf64-dev b7411a807..e8c25c225`. Ported to
`xmhf64 bae7e3943`. The configuration option is called
`--with-extra-ap-init-count`, or `--iss` in `build.sh`.

## Fix

`xmhf64 b97448c0f..bc0f05882`
* Support the guest performing INIT-SIPI-SIPI APs multiple times
* Initialize "guest activity state" VMCS field

