# Support UEFI

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

