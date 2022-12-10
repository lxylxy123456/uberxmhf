# XMHF Optimizations (XSETBV, MSR bitmap)

## Scope
* All configurations, especially nested virtualization
* `xmhf64 0d55a8144`

## Behavior

XMHF is slow. We need to try to improve its efficiency.

## Debugging

### XSETBV

As discovered in `bug_096`, QEMU will perform XSETBV during each VMEXIT /
VMENTRY. We try to improve efficiency by removing support for XSETBV in the
host.

In `xmhf64-dev 087cfc39d`, add configuration option `--enable-hide-xsetbv` to
allow removing XSETBV feature. However, this does not work because see Linux
kernel panic.

So we are not going to remove XSETBV.

### MSR Bitmap

Newer hardware supports MSR bitmap. XMHF should use it to reduce VMEXIT101 due
to WRMSR / RDMSR.

TODO

