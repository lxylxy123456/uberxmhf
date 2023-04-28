# Possible Intel TXT vulnerability about ACPI change

## Scope
* DRT
* `xmhf64 4aad53cd6`
* `xmhf64-dev 6f9b7c7f4`

## Behavior
If before DRT, some malware changes ACPI table, it is possible to modify ACPI
tables to hide the DRHD from XMHF. Then a malicious red OS can use DRHD to set
up device access to XMHF memory.

## Debugging

In `xmhf64-dev d9ef1ce89`, try to change ACPI RSDT. This time only change the
length field. Test on 2540p. See XMHF SL never starts, but hardware resets
after BL tries to launch SL. See Intel TXT error with
`TXT.ERRORCODE: 0xc0000ca1`. This is "RSDT ACPI table checksum invalid".

Looks like Intel is likely not reloading ACPI memory with flash. Thus, our
attack is likely successful.

In `xmhf64-dev 5e594554f`, try to hide DMAR table (replace name with XMHF).
However, see `TXT.ERRORCODE: 0xc00010a1`, "could not find VT-d DMAR ACPI table".

In `xmhf64-dev 6bd0bcb23`, try to modify DMAR table and swap address of two
DRHDs. However, see `TXT.ERRORCODE: 0xc00018a1`,
"BARs in VT-d DMAR DRHD struct mismatch".

## Summary

Looks like the hardware / AC module has some sanity check to make sure that
ACPI fields related to DRHD are not modified. If modified, DRT will fail to
launch. Thus, I think the attack is not possible. i.e., what I found is not a
vulnerability.

Git
* `xmhf64-dev 6f9b7c7f4..bd4c54e88`

