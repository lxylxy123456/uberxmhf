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

TODO: monitor DRHD fault register
TODO: read Linux code in `drivers/iommu/intel/iommu.c`
TODO: print Linux and XMHF interaction with DRHD
TODO: print memory dump at VMEXIT
TODO: use I/O VMEXIT

