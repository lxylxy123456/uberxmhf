# XMHF does not support guest sending INIT-SIPI-SIPI to "All Excluding Self"

## Scope
* All subarchs
* Running LHV in XMHF
* XMHF git `14dc5b41c`, lhv git `67f4a1b08`

## Behavior
For example, when booting x64 XMHF then x86 LHV, see assertion error in XMHF:

```
0x0010:0x01e01f13 -> (ICR=0x00000300 write) INIT IPI detected and skipped, value=0x000c4500Done.
Sending SIPI-0...
0x0010:0x01e01f64 -> (ICR=0x00000300 write) STARTUP IPI detected, value=0x000c4610
Fatal: Halting! Condition '(icr_low_value & 0x000C0000) == 0x0' failed, line 128, file arch/x86/vmx/smpg-x86vmx.c
```

Using gdb, can see that `icr_low_value = 0xc4610`

## Debugging

Bit 18 and 19 in ICR are "Destination Shorthand":
* 00: No Shorthand
* 01: Self
* 10: All Including Self
* 11: All Excluding Self

XMHF assumes that the guest uses 00 (i.e. specify destination in
"Destination Field", destination is a single CPU). However, XMHF itself uses
11 (destination is all CPUs excluding self). This is a feature that should be
supported.

The file to be changed is `smpg-x86vmx.c`

Fixed in `a65ee0d63`.

## Fix

`14dc5b41c..a65ee0d63`
* Allow guest to send INIT-SIPI-SIPI to "All Excluding Self"

