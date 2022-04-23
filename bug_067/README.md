# Support Intel Microcode Update

## Scope
* Observed on physical machine only
* Observed on Fedora and Windows 10, not Debian
* Git `1a3f267cd`

## Behavior
The Microcode update is first discovered in `bug_039`. The microcode update
causes page fault, so we disabled the "Hypervisor Present" bit to workaround
the problem. Now we want to support microcode update instead.

## Debugging

Again, we need to read Intel v3 "9.11 MICROCODE UPDATE FACILITIES".
"Table 9-7. Microcode Update Field Definitions" defines the format of microcode
updates, and it looks like we just need to parse the "Data Size" and "Total
Size".

Our plan is
1. Read the total size from guest update data (gva)
2. Copy the update data content (gva) to hypervisor (hva)
3. Compute checksum for the update data (hva), check whether match hard-coded
   value
4. Call microcode update on the copied update data

If any of the above fails, abort.

The main technical problems to solve are
* Be able to walk EPT and guest page table
* Be able to compute checksum

### Review HPT

TrustVisor uses a lot of page table operations. The library it uses is called
"hpt" and "hptw". These probably stand for host page table. They are not
well-documented, so now it's time to add some documentation.


