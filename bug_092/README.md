# Fix VGA broken issue

## Scope
* DMAP only
* i386 and amd64 XMHF
* Debian 11
* HP 2540P

## Behavior
When DMAP is enabled, when Debian 11 is booting on HP 2540P, the VGA output
becomes unreadable after the VGA resolution is increased. However, sometimes
the booting process can continue and SSH even works. Then when GNOME starts,
the problem disappears.

## Debugging

Information from Miao
* Documentation
	* Intel(R) Virtualization Technology for Directed I/O
	* Architecture Specification
	* October 2014
	* Order Number: D51397-007, Rev. 2.3
	* Sections
		* 3.1 - 3.5: introduction
			* 3.4.3 Context-Entry: how two page tables look like
				* Linux lspci result is "BUS:DEV.FUNC"
				* In XMHF, the page tables are called "RET" and "CET"
		* 3.7 Second-Level Translation: used by XMHF
		* 9.1, 9.3
	* XMHF code
		* In `xmhf-startup/runtime.c`, `#if defined (__DMAP__)` is the init
		  logic
		* Remove `xmhf_dmaprot_invalidate_cache()` (previous workaround)
		* Remove `xmhf_dmaprot_protect()` at line 217, should then be still be
		  reproducible
		* Tried changing cache, not good
		* Tried changing "Translation Type" in "Context Entry" to
		  "Untranslated requests are processed as pass-through", not good
		* Not likely to be related to extended

We download documentation from
<https://lettieri.iet.unipi.it/virtualization/vt-directed-io-spec.pdf>.

To reproduce this issue, use `./build.sh amd64 fast --dmap && gr`. When DMAP is
disabled, the issue no longer appears.

If compile with DMAP, but remove `__DMAP__` related code in `runtime.c` and
`sl.c`, the bug also disappears.

I also just realized that no only does Debian 11's VGA output corrupt, the GRUB
graphical mode's VGA output is also corrupted. This can be considered an easy
way to test this problem. Grub's VGA mode is called "gfxterm" in source code.

Found a problem in `xmhf_sl_arch_early_dmaprot_init()`, where a hardcoded
address is used. After more code reading and discussion, looks like this is the
middle of `.sl_untrusted_params` section. This section does not contain a lot
of information. The code should be fine in the sense of security, though the
hardcoded memory is strange. Should formally define an array in the
`.sl_untrusted_params` section.

Useful references
* <https://www.kernel.org/doc/Documentation/Intel-IOMMU.txt>

There are two major places where XMHF calls DMAP. First in `sl.c` function
`xmhf_sl_arch_early_dmaprot_init()`. The main logic is in
`vmx_eap_initialize_early()`. Here the jobs done are
* Walk ACPI, get DRHDs
* Set up RET and CET (memset to 0), so that no DMA is allowed
* For all DRHDs, initialize registers (e.g. RET address)

In runtime, a similar logic is executed. So if
`xmhf_sl_arch_early_dmaprot_init()` is not called, the bug is still
reproducible. See git `xmhf64-dev c80c36b51`.

Major functions called in runtime are
* `xmhf_iommu_init()`: just calls malloc, not interesting
* `xmhf_dmaprot_initialize()`
* `xmhf_dmaprot_protect()`: already disabled
* `xmhf_dmaprot_enable()`
* `xmhf_dmaprot_invalidate_cache()`

Now we experiment changing `_vtd_setuppagetables()` and see behavior
* Current: all pages are mapped, EMT=0
* Set EMT=6 (WB): same behavior
	* Looks like EMT is ignored, because ecap.MTS=0. So this bug is likely not
	  related to MTRR
* Only map memory >= 1M: same behavior as map all
* Only map memory < 1M: GRUB does not start (only prints "GRUB")
* Only map memory 0-0xa0000 and 0xc0000-1M: same behavior as map all
* Only map memory 0-0xa0000: same behavior as map all
* Only map memory 0-0x80000: GRUB does not start (only prints "GRUB")
* Only map memory 0x10000-0xa0000: same behavior as map all
* Only map memory 0x80000-0xa0000: print "GRUB", then triple fault
* Only map memory 0x40000-0xa0000: same behavior as map all
* Only map memory 0x60000-0xa0000: same behavior as map all
* Only map memory 0x70000-0xa0000: prints "GRUB", then "no such partition"
* Only map memory 0x70000-0xa0000 readonly: print "GRUB", then triple fault
* Only map memory 0x70000-0xa0000 writeonly: print "GRUB", then stuck
* Only map memory 0x60000-0xa0000 readonly: print "GRUB", then triple fault
* Only map memory 0x60000-0xa0000 writeonly: print "GRUB", then stuck

We print all VMEXITs and their CS:RIP to get a better picture. However this
does not help too much.

Then we remove entries in RET and CET and see what happens. Change the
`_vtd_setupRETCET()` function.
* Only map [0, 16) in RET: same behavior as map all
* Only map [0, 4) in RET: same behavior as map all
* Only map [0, 1) in RET: same behavior as map all
* Only map [1, 256) in RET: print "GRUB", then stuck, no VMEXIT
* Only map 0 in RET, [0, 128) in CET: print "GRUB", then stuck, no VMEXIT
* Only map 0 in RET, (128, 256) in CET: same behavior as map all
* Only map 0 in RET, (128, 192) in CET: print "GRUB", then stuck, no VMEXIT
* Only map 0 in RET, [224, 256) in CET: same behavior as map all
* Only map 0 in RET, [240, 256) in CET: same behavior as map all
* Only map 0 in RET, [248, 256) in CET: same behavior as map all
* Only map 0 in RET, [252, 256) in CET: print "GRUB", then stuck, no VMEXIT
* Only map 0 in RET, [248, 250) in CET: print "GRUB", then stuck, no VMEXIT
* Only map 0 in RET, 250 in CET: same behavior as map all
	* See `xmhf64-dev dbeba0a95`

This correcponds to `00:1f.2` in lspic, which is
```
00:1f.2 IDE interface: Intel Corporation 5 Series/3400 Series Chipset 4 port SATA IDE Controller (rev 05)
```

In GRUB source code, looks like the most likely related function is
`grub_ata_readwrite()`. It uses macro `GRUB_ATA_CMD_READ_SECTORS_DMA_EXT`,
which is the same as <https://wiki.osdev.org/PCI_IDE_Controller>.

We also remove DRHDs. There are 3 DRHDs in total.
* Only configure DRHD 0: no bug reproduced
* Only configure DRHD 0 and 1: same behavior as map all
* Only configure DRHD 1: same behavior as map all. See `xmhf64-dev de0fa3d4a`.

Currently we guess the bug happens at:
* DRHD 1 (index starts at 0)
* ACPI 00:1f.2 (IDE)
* Memory 0x60000-0xa0000
* Before the first VMEXIT

In `xmhf64-dev 93ae57d1b`, we remove VT-d after the first VMEXIT
(CS:RIP=0x8:9321, reason=10 CPUID). The behavior of GRUB is still the same.
After GRUB boots Debian, Debian seems to stuck at a different place.

Now my guess about this bug is that when IDE is reading disk sectors to memory,
the data gets corrupted. This causes screen corruption as a by-product. The
next step is to dump memory and compare DMAP / no DMAP behavior.

### Linux message about firmware bug

```
DMAR: Host address width 36
DMAR: DRHD base: 0x000000fed90000 flags: 0x0
DMAR: dmar0: reg_base_addr fed90000 ver 1:0 cap c9008020e30272 ecap 1000
DMAR: DRHD base: 0x000000fed91000 flags: 0x0
DMAR: dmar1: reg_base_addr fed91000 ver 1:0 cap c0000020230272 ecap 1000
DMAR: DRHD base: 0x000000fed93000 flags: 0x1
DMAR: dmar2: reg_base_addr fed93000 ver 1:0 cap c9008020630272 ecap 1000
DMAR: RMRR base: 0x00000000000000 end: 0x00000000000fff
DMAR: RMRR base: 0x000000bdc00000 end: 0x000000bfffffff
DMAR: [Firmware Bug]: No firmware reserved region can cover this RMRR [0x00000000bdc00000-0x00000000bfffffff], contact BIOS vendor for fixes
DMAR: [Firmware Bug]: Your BIOS is broken; bad RMRR [0x00000000bdc00000-0x00000000bfffffff]
BIOS vendor: Hewlett-Packard; Ver: 68CSU Ver. F.22; Product Version: 
```

However, looks like this bug is not related to our bug. The memory region is
well above 1MB.

### Dumping memory

We dump memory at the first VMEXIT. Before VMENTRY, set all memory of interest
to 0xc5. Git `xmhf64-dev 8d0c95878`, DMAP serial `20220910154438` and
`20220910154758`, no DMAP serial `20220910154526` and `20220910154845`.

We can see that when DMAP is enabled, around 18000 lines change. However,
normally only around 100 lines change between different runs with the same
configuration.

Using the memory map and look for pages full of 0xc5, we further reduce VT-d
page table size
* Only map memory 0x68000-0xa0000: same behavior as map all
* Only map memory 0x70000-0xa0000: same behavior as map all
* Only map memory 0x70000-0x80000 and 0x90000-0xa0000: same behavior as map all
* Only map pages 0x70-0x7c, 0x7f, 0x9f: same behavior as map all
	* See git `xmhf64-dev c2aed82a9`

At this point, I realized that I was looking at the bad one's memory. The
difference is that memory of the bad one is 0xc5, but the good one is
0 or non-trivial value. We still need to dump 0x68000-0xa0000.

In `xmhf64-dev c2020ffc7`, cause VMEXIT on all I/O instructions. Two I/O
instructions are too frequent and are removed from printf result:
* CS:EIP=0xf000:0x0000feeb, exit qualification=0x00200040, port=0x0020
* CS:EIP=0xf000:0x0000fa46, exit qualification=0x00610048, port=0x0061

Git `xmhf64-dev 1e44ef8de`, DMAP serial `20220910195957` and `20220910200536`,
no DMAP serial `20220910204328`. This time stop at the first VMEXIT to make
things simple. Can see that the VMEXITs are the same regardless of whether DMAP
is enabled. Now we dump some memory at VMEXIT.

Actually I realized that the previous code are wrong. Clearing memory with 0xc5
only happens when DMAP is enabled. This bug is fixed in git
`xmhf64-dev 910c2fe90`, DMAP serial `20220910205732`, no DMAP serial
`20220910205804`. Now even if we dump all memory between 0x60000-0xa0000, DMAP
and no DMAP have the same memory.

After reviewing code for `_vtd_drhd_initialize()`, found a typo in 3 places
where `VTD_REG_WRITE` should be `VTD_REG_READ`. Fixed in `xmhf64 c510f195f`.
Also realized that in `xmhf64` branch, GRUB VGA is good. But in `xmhf64-dev`
branch, GRUB VGA also corrupts.

### Delay enable of DMAP

I realized that if we call `xmhf_dmaprot_enable()` after some VMEXITs, the
bug can still be reproducible. This can be used to find the relevant GRUB code
that causes this bug.

`grep VMEXIT 20220910154438 | cut -b 1-50 | uniq -c` shows the list of VMEXITs.
They are:
1. CPUID at 0x00009321
2. E820 (19 times)
3. CPUID at 0x00009ae7
4. CPUID at 0x00009af5
5. CPUID at 0x00009b36
6. CPUID at 0x00009b7b
7. CPUID at 0x00009a95 (76 times)
8. CPUID at 0x0fecf1ac
9. RDMSR at 0x0fecf1bd
10. CPUID at 0x0fecf1d7

* Enable DMAP at 1: same behavior as enable in `runtime.c`
* Enable DMAP at 5: same behavior as enable in `runtime.c`
* Enable DMAP at 8 (RIP=0x0fecf1ac): same behavior as enable in `runtime.c`
* Enable DMAP at 10 (RIP=0x0fecf1d7): same behavior as enable in `runtime.c`
* Enable DMAP at RIP=0x0fecf311 (index=42): same behavior as enable in
  `runtime.c`
* Enable DMAP at RIP=0x01674d69 (index=271): no bug reproduced
* Enable DMAP at index=156: no bug reproduced
* Enable DMAP at index=74: at first the VGA is good. Looks like the VGA is
  corrupted as soon as DMAP is enabled.

In `xmhf64-dev fb11448ea`, print index to better locate a VMEXIT. Serial
`20220911131101`. VMEXITs at RIP=0x00009a95 are removed because they are likely
timer-dependent. At least for index < 300, VMEXITs are reproducible.

For the index=74 experiment, if we halt in hypervisor immediately after DMAP
is enabled, still see the VGA corruption. Git `xmhf64-dev bb569fa1c`.

Now my guess is that VGA accesses memory using DMA. However during debugging,
only IDE access to 0x60000-0xa0000, so during GRUB VGA access is blocked. So
GRUB VGA corrupts. If I revert such changes, GRUB VGA is good but Debian VGA is
still corrupted. Now we check which device causes the GRUB VGA to corrupt

* Only map 0 in RET, all in CET: VGA does not corrupt
* Only map 0 in RET, [0, 128) in CET: VGA does not corrupt
* Only map 0 in RET, [0, 48) in CET: VGA does not corrupt
* Only map 0 in RET, [0, 28) in CET: VGA does not corrupt
* Only map 0 in RET, [0, 8) in CET: VGA corrupts
* Only map 0 in RET, [8, 18) in CET: VGA does not corrupt
* Only map 0 in RET, [8, 13) in CET: VGA corrupts
* Only map 0 in RET, [16, 18) in CET: VGA does not corrupt
* Only map 0 in RET, 16 in CET: VGA does not corrupt
* Only map 0 in RET, 16 in CET, memory 0-0x100000: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0x100000-0x80000000: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xc0000000-4G: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0-4G: VGA does not corrupt
* Only map 0 in RET, 16 in CET, memory 0x100000-4G: VGA does not corrupt
* Only map 0 in RET, 16 in CET, memory 0x80000000-0xc0000000: VGA does not
  corrupt
* Only map 0 in RET, 16 in CET, memory 0x80000000-0xa0000000: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xa0000000-0xb0000000: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xb0000000-0xc0000000: VGA does not
  corrupt
* Only map 0 in RET, 16 in CET, memory 0xb8000000-0xc0000000: VGA does not
  corrupt
* Only map 0 in RET, 16 in CET, memory 0xbc000000-0xc0000000: VGA does not
  corrupt
* Only map 0 in RET, 16 in CET, memory 0xbf000000-0xc0000000: VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xbe000000-0xbf000000, stop at index=57:
  VGA does not corrupt
* Only map 0 in RET, 16 in CET, memory 0xbe800000-0xbf000000, stop at index=57:
  VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xbe000000-0xbe400000, stop at index=57:
  VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe800000, stop at index=46:
  VGA corrupts
* Only map 0 in RET, 16 in CET, memory 0xbe000000-0xbe800000, stop at index=46:
  screen is blank. Looks like index=46 is too early
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe800000, stop at index=50:
  The first around 4.2 rows are corrupted.
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe600000, stop at index=50:
  The first around 4.2 rows and the second half ot the screen are corrupted.
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe600000 read only, stop at
  index=50: the entire screen is corrupted
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe600000 write only, stop at
  index=50: the entire screen is corrupted
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe700000, stop at index=54:
  the entire screen is corrupted
* Only map 0 in RET, 16 in CET, memory 0xbe400000-0xbe700000, stop at index=50:
  The first few rows and the last quarter of the screen are corrupted.

The updated story is: VGA hardware uses DMA to access memory from
0xbe400000-delta1 to 0xbe600000-delta2 and map the memory to the screen. If the
memory cannot be accessed, some strange value is "returned" and the screen
displays this value.

The device in lspci is:
```
00:02.0 VGA compatible controller: Intel Corporation Core Processor Integrated Graphics Controller (rev 02)
```

### Fault logging

Looks like the Fault Recording Registers (FRR) etc will be helpful for us when
debugging this problem. Before using it, need to make sure that PFO in Fault
Status Register (FSR) is 0 (write 1 to clear). Otherwise, the hardware will not
log to the FRR. This is something not implemented well in
`_vtd_drhd_initialize()`. I think FRR.F may also need to be cleared.

The hardware logic is in "7.3.1 Primary Fault Logging". It specifies how
software should clear FRR.F and then FSR.PFO

For example, during the GRUB VGA corruption, FRR becomes
`FRR=0x8000000500000010:0x00000000be000000`. This is `FR=5`, `SID=0x0010`, and
`FI=0x00000000be000000`. The fault reason is can be looked up in appendix A:
> DMA Remapping Fault Conditions:
> A non-recoverable address translation fault resulted due to lack of write
> permission.

The good news is that the lower 64 bits of FRR (`0x00000000be000000`) indicates
the address (page granularity) that caused the fault. This is very helpful for
debugging.

Using FRR, I realized that there are more memory that are accessed by VGA
* Only map 0xbe000000-0xb7000000: fault at 0xb7000000, part of screen corrupted
	* Git `xmhf64-dev 5e640aef1`
* Only map 0xbe000000-0xb8000000: fault at 0xb8000000
* Only map 0xbe000000-0xb9000000: fault at 0xb9000000
* Only map 0xbe000000-0xbf000000: fault at 0xbf000000
* Only map 0xbe000000-0xc0000000: no fault

I think we can stop here on GRUB and focus on Debian. However, it also means
that `### Linux message about firmware bug` may be related, because the memory
region identified by Linux is 0xbdc00000-0xbfffffff.

### Why GRUB VGA corrupts

Strangely, GRUB VGA corrupts at unexepcted code.
* `xmhf64-dev 60ed5431f`: corrupt (unexpected)
* `xmhf64 c510f195f`: not corrupt
* `60ed5431f`: corrupt
* `a1841d497`: corrupt
* `40b8bb624`: corrupt
* `58692ce84`: corrupt
* `eb66d8833`: not corrupt

Actually the bug happens after removing the following DMAP flush workaround in
`runtime.c`. So should not use `xmhf64` branch as the baseline.
```c
#if defined (__DMAP__)
  // [TODO][Superymk] Ugly hack: HP2540p's GPU does not work properly if not invoking <xmhf_dmaprot_invalidate_cache> 
  // in <xmhf_runtime_main>.
  xmhf_dmaprot_invalidate_cache();
#endif
```

This means that GRUB can reproduce the bug, or there are 2 bugs. In
`xmhf64-dev 661334b41..66e10f1d9`, try to bisect XMHF code logic and see where
the bug starts to happen. However, not successful.

Then we go back to fault logging. See `xmhf64-dev 5789b937a`. Looks like there
is a fault right after enabling GCMD.TE, where
FRR=0x0000000100000010:0x00000000be000000. However, this does not make sense
because the cache should be flushed correctly.

Now the best bet is to see how Debian initializes VT-d and compare the behavior
with XMHF.

### Write buffer flush

While reviewing the documentation, I realized that there is a global command
called "WBF: Write Buffer Flush \[1\]". The comment says:
> 1. Implementations reporting write-buffer flushing as required in Capability
> register must perform implicit
> write buffer flushing as a pre-condition to all context-cache and IOTLB
> invalidation operations.

Linux code (`drivers/iommu/intel/iommu.c`) also shows the use of WBF, and XMHF
code already identifies that "VT-d hardware access to remapping structures
NON-COHERENT".
```c
void iommu_flush_write_buffer(struct intel_iommu *iommu)
{
	u32 val;
	unsigned long flag;

	if (!rwbf_quirk && !cap_rwbf(iommu->cap))
		return;

	raw_spin_lock_irqsave(&iommu->register_lock, flag);
	writel(iommu->gcmd | DMA_GCMD_WBF, iommu->reg + DMAR_GCMD_REG);

	/* Make sure hardware complete it */
	IOMMU_WAIT_OP(iommu, DMAR_GSTS_REG,
		      readl, (!(val & DMA_GSTS_WBFS)), val);

	raw_spin_unlock_irqrestore(&iommu->register_lock, flag);
}
```

So we should flush write buffer. A PoC is in `xmhf64-dev 0a9f9163e`, which
flushes WB very frequently. Looks like the GRUB VGA corruption issue is solved.
* Before the commit: `5789b937a`, GRUB VGA corrupts
* After the commit: `0a9f9163e`, GRUB VGA is good

After some experiments, looks like this WBF need to be done after
`// 8. enable device`. Also read documentation "6.8 Write Buffer Flushing".
This looks reasonable.

Also, we review things done in `_vtd_drhd_initialize()`.
```c
    // 3. setup fault logging
    // 4. setup RET (root-entry)
    // 5. invalidate CET cache
    // 6. invalidate IOTLB
    // 7. disable options we dont support
    // 8. enable device
    // 9. disable protected memory regions (PMR) if available
```

Note that in 9, currently PMR is already disabled. This may change if we switch
from `xmhf64-dev` to `xmhf64`.

After performing the WBF, looks like all DMAP issues on 2540p are gone. Git
`xmhf64 569abea18`.

Untried ideas
* Print Linux and XMHF interaction with DRHD
	* Can use XMHF to do that, inject some exception to let Linux print call
	  stack

### Code clean-up

TODO: `_vtd_drhd_initialize()` does not clear FSR.PFO and FRR.F correctly
TODO: `_vtd_invalidatecaches()` may need to wait: see `xmhf64-dev c1e749be7`
TODO: be able to justify where exactly WBF should be performed

## Fix

`xmhf64 1c3d520a9..2c767d24b`
* Fix typo in `dmap-vmx-internal.c`
* Perform WBF in `_vtd_drhd_initialize()`

