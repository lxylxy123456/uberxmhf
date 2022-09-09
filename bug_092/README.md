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

TODO: remove entries in RET and CET

