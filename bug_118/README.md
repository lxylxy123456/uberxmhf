# Vulnerability due to Intel PT

## Scope

* Hardware
	* Not on HP 2540p (does not support Intel PT)
	* On Dell 7050 (supports Intel PT)
* i386 and amd64
* `xmhf64 4aad53cd6b`
* `xmhf64-dev bd4c54e88`
* `lhv db7157160b`
* `lhv-dev df6578f6ce`

## Behavior

XMHF does not block Intel PT related MSRs (start with `IA32_RTIT_`). This may
allow a malicious OS to write to XMHF memory.

## Debugging

One way is to hide Intel PT. MSRs to be blocked are
* 0x560 `IA32_RTIT_OUTPUT_BASE`
* 0x561 `IA32_RTIT_OUTPUT_MASK_PTRS`
* 0x570 `IA32_RTIT_CTL`
* 0x571 `IA32_RTIT_STATUS`
* 0x572 `IA32_RTIT_CR3_MATCH`
* 0x580 `IA32_RTIT_ADDR0_A`
* ...
* 0x587 `IA32_RTIT_ADDR3_B`

Another way is to virtualize Intel PT. Use VMCS
"Intel PT uses guest physical addresses" bit. By setting this bit to 1, the
guest can no longer use `IA32_RTIT_OUTPUT_BASE` to modify XMHF memory. Also
need to force this bit to 1 in nested virtualization.

Fixed in `xmhf64 798340e69`. Not testing on Dell 7050 because recently
installed UEFI on this machine. Cannot test on HP 2540P because it does not
support Intel PT. However, tested on QEMU to make sure CPUID bit is correct.

Later, Jenkins CI shows that when running (XMHF i386, XMHF i386, Debian 11),
for some reason Debian 11 performs RDMSR `IA32_RTIT_CTL` for unknown reason.
This results in XMHF halting. In `xmhf64 36fad3310`, we relax the restriction.
Only block WRMSR, and do not block RDMSR.

## Fix

`xmhf64 4aad53cd6..36fad3310`
* Hide Intel PT (not a lot of work to virtualize, though)

