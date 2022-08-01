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

In `xmhf64-nest 84b01ca77..3c6192ea0`, allow using VMCS shadowing. However,
there is a bug in VMCS shadowing when compiled with `-O3` (found by CI). The
version before the commits (`xmhf64-nest 84b01ca77`) look good.

Also when trying to compile XMHF with `-O3` on Fedora, found that there is a
known bug in GCC (since Fedora recently updated GCC to version 12):
<https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104657>. To workaround this GCC
bug, add `-Wno-array-bounds` to `--with-opt=`.

The problem with XMHF's `-O3` bug is that a strange VMEXIT happens. So we
compare VMCS content between `-O0` and `-O3`. Use any VMEXIT before VMWRITE to

Actually, this problem happens because `control_VMREAD_bitmap_address`
and `control_VMWRITE_bitmap_address` are 0. After some code review it looks
like `__vmx_vmwrite` is used, instead of `__vmx_vmwrite64`. For some reason
`-O3` optimization will encounter problems. Fixed in `xmhf64-nest 5af7fe209`.

### VMXOFF

There are a few instructions not yet implemented: VMXOFF, VMPTRLD. In
`lhv 375bbf44f..85fd58a39`, add `LHV_OPT` 0x40 to use these instructions.
In `xmhf64-nest 5af7fe209..98696d2e2`, implemented VMXOFF, VMPTRLD.

### Testing on x86 XMHF

Now we try KVM XMHF XMHF Debian x86 again. The commands are:

```sh
./build.sh i386 fast --sl-base 0x20000000 circleci --no-init-smp && gr
./build.sh i386 fast circleci && gr
(cd build32 && make -j 4 && gr) && (cd build64 && make -j 4 && gr) && tmp/bios-qemu.sh -smp 2 -m 1G --gdb 2198 -d build32 +1 -d build64 +1 -d debian11x86 +1
```

After waiting for a long time, see VMRESUME failure in L1 XMHF, CPU 0 (CPU 1 is
waiting for SIPI in L2 XMHF). In a newer host machine the waiting time is
shorter. See `bad1.txt` for the VMCS dump. Before the first VMENTRY, we dump
the VMCS as `good1.txt`.

Notice that when the VMENTRY failure happens, there is VMENTRY event injection:
```
CPU(0x00): (0x4016) VMCS02.control_VM_entry_interruption_information = 0x80000603
CPU(0x00): (0x4018) VMCS02.control_VM_entry_exception_errorcode = 0x00000000
CPU(0x00): (0x401a) VMCS02.control_VM_entry_instruction_length = 0x00000000
...
CPU(0x00): (0x4408) VMCS02.info_IDT_vectoring_information = 0x80000603
CPU(0x00): (0x440a) VMCS02.info_IDT_vectoring_error_code = 0x00000000
CPU(0x00): (0x440c) VMCS02.info_vmexit_instruction_length = 0x00000001
```

The problem is that when copying event injection from VMEXIT to VMENTRY, the
instruction length also needs to be copied. If change `0x401a` to `0x1`, the
VMENTRY error no longer happens.

In Intel v3 "26.2.5 Information for VM Exits Due to Instruction Execution",
looks like "VM-exit instruction length" should also be copied. Fixed in
`xmhf64-nest e3e687ca1..d525e5d83`.

Also, we walked Linux's page table (by changing `gdb/page.gdb` a little bit),
and it turns out that the L3 Debian indeed executes the INT3 instruction.

After fixing this bug, HP 840 can boot KVM XMHF XMHF Debian correctly in a
relatively short amount of time. For Thinkpad we need to wait for a long time,
though. Note that we still need to remove the verbose message printed, so use
`xmhf64-nest-dev fcabd90a1` instead of `xmhf64-nest d525e5d83`.

### Saving EPT violations

Another idea to improve performance is to cache EPT violation history. When
INVEPT is executed, re-run this history to restore EPT02 without triggering EPT
violations from L2.

This is implemented on `xmhf64-nest` branch in `8cae6e5b0..21a478bf2`. To test
it, use `xmhf64-nest-dev 6fad3c4e4`. However, looks like there is not
significant performance improvement on Thinkpad (still need to wait for a long
time). So not committing to `xmhf64-nest`.

### Support large pages

TODO: add tests in LHV

