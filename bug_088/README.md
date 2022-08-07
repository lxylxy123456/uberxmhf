# Running SMP Linux in XMHF in XMHF (more optimizations)

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

### Support large pages in EPT12

At this point, L0 (XMHF) does not use large pages. However, at least it should
allow L1 guest to use large pages. First we add test in LHV to use large pages.

First, in `xmhf64 330e650f4..b87a0dcdd`, add a function in `hpt.c` to be able
to set page size bit. Then, in `lhv 254fa80a4..2a0734e69`, test large pages by
swapping two 2M pages. In `xmhf64-nest 6355bd66f..e1fe73dc8`, able to handle
large pages.

### Invalidating EPT02 when EPT01 is invalidated

We realized that to be correct, when EPT01 changes, EPT02 also needs to be
invalidated.

There are 3 interfaces to invalidate EPT01 in XMHF:
* `xmhf_memprot_flushmappings()`: only invalidate one EPT
* `xmhf_memprot_flushmappings_localtlb()`: invalidate all EPTs on current CPU
* `xmhf_memprot_flushmappings_alltlb()`: invalidate all EPTs on all CPUs

First, in `xmhf64 8968e4c63`, update TrustVisor to use
`xmhf_memprot_flushmappings_alltlb()`.

Then, in `xmhf64-nest 5876a6f8d`, make `xmhf_memprot_flushmappings()` the same
as `xmhf_memprot_flushmappings_localtlb()`. Then let
`xmhf_memprot_flushmappings_localtlb()` call nested virtualization code to
invalidate all EPT02s.

Then, I reverted the above commit in `xmhf64-nest 06a387f60` because it has a
number of problems. The problems first arised when CI failed, because WRMSR to
MTRRs flushes EPT. When this is done automatically in VMENTRY and VMEXIT, bad
things will happen.

We need to write this code more carefully. The things to consider are
* `xmhf_memprot_flushmappings_alltlb()` will be called when NMI arrives, but
  there are inconvenient times where a CPU cannot take EPT02 violations, like
  in `xmhf_nested_arch_x86vmx_handle_ept02_exit()`. Need to allow the CPU to
  delay flushing EPT02, as in delay processing NMIs that should be injected to
  the guest.
	* `ept02_cache` need to be protected, affecting the following functions
		* `xmhf_nested_arch_x86vmx_ept_init()` (executed before guest, no need
		  to worry)
		* `xmhf_nested_arch_x86vmx_invept_*()`
		* `xmhf_nested_arch_x86vmx_get_ept02()`
	* `vmcs12_info->guest_ept_cache_line` and `vmcs12_info->guest_ept_root`
	  need to be proctected, affecting
		* `xmhf_nested_arch_x86vmx_handle_vmexit()` (EPT handling part).
	* VMCS02's `control_EPT_pointer`
* MTRR logic should be changed. EPT02's EPT MT should inherit from EPT12,
  instead of combining EPT01 and EPT12. In other words, `ept_merge_hpt_pmt()`
  should not exist, because Intel manual does not define a way of combining
  EPT and MTRRs.
* For MTRR WRMSR handler, should call a function that only invalidates EPT01.
  For other callers, should call a function that invalidates both EPT01 and
  EPT02.
* In `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`, call to
  `xmhf_nested_arch_x86vmx_get_ept02()` is required to have cache hit. However,
  since EPT02 may be invalidated asynchronously, this assertion need to be
  removed.
	* Or an alternative implementation is to build an empty EPT02 when
	  asynchronously flushing EPT02.

The new implementation is in `xmhf64-nest 06a387f60..6041be5ad`. Basically this
implementation covers what are discussed above. After the change, looks like
KVM XMHF XMHF Debian still works.

### Simplify LHV

Currently the VMCALL and guest logic of LHV is complicated. We change LHV so
that the logic becomes similar to NMI test cases. That is, the state of the
guest is tracked using RIP. The state of the host is tracked by CPU global
variables (in this case a function pointer).

In `lhv 2a0734e69..959d71e28`, implement the above. We also test presence of
XMHF from LHV. The benefit is that Jenkins no longer relies on XMHF's output.
So the printf of nested VMENTRY and VMEXIT can be removed. See
`xmhf64-nest 6041be5ad..96483829b`.

We also add testing of user mode in LHV, but now test fails because of some
CPUs stuck in XMHF (similar to deadlock).

* Bad: SMP 4, `LHV_OPT = 0x1fd`
* Good: SMP 4, `LHV_OPT = 0x1bf`

Inaccurate results, because 0x40 and 0x2 are exclusive
* Bad: SMP 4, `LHV_OPT = 0x1ff`
* Bad: SMP 2, `LHV_OPT = 0x1ff`
* Bad: SMP 4, `LHV_OPT = 0xfe`
* Bad: SMP 4, `LHV_OPT = 0x4e`
* Bad: SMP 2, `LHV_OPT = 0x42`

In `lhv a8d8fbd67..d72c991ab`, implement `LHV_NO_INTERRUPT = 0x200` to remove
all interrupt sources but not remove EFLAGS.IF.
* Bad: SMP 4, `LHV_OPT = 0x3fd`
* Bad: SMP 4, `LHV_OPT = 0x34d`
* Bad: SMP 4, `LHV_OPT = 0x244`
* Bad: SMP 2, `LHV_OPT = 0x244`
* Good: SMP 4, `LHV_OPT = 0x240`
* Good: SMP 1, `LHV_OPT = 0x244`

So we can see that the problem happens when EPT and user mode are both enabled.
Also need to be SMP.

Using GDB, we can see that the CPU stucks at LHV because of an unexpected EPT
exit. In `lhv-dev 6d4b64418` we change LHV a little bit to make EPT exit
impossible. Then in `xmhf64-nest-dev 925a8f17b` we try to detect this problem
in XMHF. We are on the right track.

We predict that the problem happens when EPT02 flush happens before handling
nested EPT violation VMEXITs. In `xmhf64-nest-dev b953e96c9`, we put a EPT02
flush in the code, and now the bug is always reproducible in 1 CPU.

TODO: fix LHV test when `LHV_OPT = 0x240`
TODO: test when XMHF also uses large page (below)

### Testing large pages in EPT01

Supporting large pages in EPT01 still sounds challenging, because that means
TrustVisor also needs to be modified. However, when TrustVisor is not in use,
we can change the EPT however we want. Simply modify `_vmx_setupEPT()`.

