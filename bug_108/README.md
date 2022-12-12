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

In `xmhf64 3b6e54256..3105d1d27`, implement MSR bitmap. Also when SMP boot of
guest is done, remove `IA32_X2APIC_ICR` VMEXIT.

Note that when nested virtualization, XMHF still intercepts all MSR VMEXITs.
This is because to implement correctly, we need to set VMCS12 MSR bitmap as
readonly in EPT. However, this requires techniques similar to shadow paging.

After the changes, event logger output is much slower because there are less
VMEXITs.

## Fix

`xmhf64 3b6e54256..3105d1d27`
* Use MSR bitmaps during L1 operations to speed up XMHF

