# PR 15 Secureloader bloat

## Scope
* During compile phase
* amd64, O3
* Repo `git@github.com:superymk/uberxmhf`, branch `mxmhf64`, commit `0b506d124`

## Behavior
GitHub actions fail because when running `build.sh amd64 release O3`, secure
loader size exceeds 64 KiB. Link:
<https://github.com/lxylxy123456/uberxmhf/actions/runs/3140439879/jobs/5101825009>

## Debugging

The compiled `sl.exe` size changes are:
```diff
   [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
   [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
   [ 1] .sl_header        PROGBITS        0000000000000000 001000 003000 00  WA  0   0 4096
-  [ 2] .text             PROGBITS        0000000000003000 004000 009125 00  AX  0   0 16
-  [ 3] .data             PROGBITS        000000000000c130 00d130 000094 00  WA  0   0 16
-  [ 4] .rodata           PROGBITS        000000000000c1e0 00d1e0 002740 00   A  0   0 32
-  [ 5] .bss              NOBITS          000000000000e920 00f920 0006f0 00  WA  0   0 32
-  [ 6] .sl_stack         PROGBITS        000000000000f010 010010 000ff0 00   A  0   0  1
+  [ 2] .text             PROGBITS        0000000000003000 004000 0093c5 00  AX  0   0 16
+  [ 3] .data             PROGBITS        000000000000c3d0 00d3d0 000094 00  WA  0   0 16
+  [ 4] .rodata           PROGBITS        000000000000c480 00d480 0027f0 00   A  0   0 32
+  [ 5] .bss              NOBITS          000000000000ec80 00fc70 000770 00  WA  0   0 32
+  [ 6] .sl_stack         PROGBITS        000000000000f3f0 0103f0 000c10 00   A  0   0  1
   [ 7] .sl_untrusted_params PROGBITS        0000000000010000 011000 1f0000 00  WA  0   0 4096
   [ 8] .shstrtab         STRTAB          0000000000000000 201000 00004e 00      0   0  1
```

Can see that the biggest change is `.text` section, with 672 bytes of change.

Adapted `bug_091/bss_size.py` and form `sl_size.py`. Then compare build sizes
of `xmhf64 710e9a847` and `mxmhf64 0b506d124`. The results are:

```diff
@@ -1,18 +1,18 @@
 181	xmhf/src/xmhf-core/xmhf-runtime/xmhf-tpm/tpm-interface.o
 2661	xmhf/src/xmhf-core/xmhf-runtime/xmhf-tpm/arch/x86/tpm-x86.o
 59	xmhf/src/xmhf-core/xmhf-runtime/xmhf-tpm/arch/x86/svm/tpm-x86svm.o
 200	xmhf/src/xmhf-core/xmhf-runtime/xmhf-tpm/arch/x86/vmx/tpm-x86vmx.o
 5	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/dmap-interface-earlyinit.o
 109	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/dmap-x86-earlyinit.o
 2533	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/svm/dmap-svm.o
-1263	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-earlyinit.o
-2297	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-internal.o
+1216	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-earlyinit.o
+2912	xmhf/src/xmhf-core/xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-internal-common.o
 37	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/bplt-interface.o
 254	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86.o
 557	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pci.o
 269	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-acpi.o
 23	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-amd64-smplock.o
 114	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.o
-20	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-cpu.o
+133	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-cpu.o
 108	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx.o
 4288	xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs.o
```

Can see that for `.text` section, `dmap-vmx-internal-common.o` changes the most
(615 bytes). I realized that `IOMMU_WAIT_OP` is still a macro, which in theory
causes the bloat.

If I modify `IOMMU_WAIT_OP` to remove waiting logic, the problem is solved. See
<https://github.com/superymk/uberxmhf/pull/8>.

Note: later, worked around this problem using something else.

### Remove unused symbols

<https://stackoverflow.com/a/6770305> shows a way to remove unused symbols
using gcc and ld. To apply to XMHF:
* Add `-fdata-sections -ffunction-sections` to CFLAGS
* Add `--gc-sections` to ld's flags (there is no LDFLAGS as of now)
* Update ld scripts

This is implemented in `xmhf64-dev 8e0f97a87`. Using `./build.sh i386 --drt`,
SL size does not decrease a lot.

```sh
readelf -SW ./xmhf/src/xmhf-core/xmhf-secureloader/sl.exe | grep .sl_stack | cut -b 41-58
```

See `Makefile`, `ld_gc_demo.c`, and `ld_gc_demo.lds`. If I add `-m32` to gcc
and change to `OUTPUT_ARCH(i386)` in ld script, the gc-sections will fail.
Looks like 64-bit is fine.

Using `./build.sh amd64 --drt`, SL size decreases from 0xae70 to 0x4820. This
is significant savings. See `nm.diff` for list of symbols removed.

Tested this on `xmhf64-tboot10 ac6ff784a`. See `xmhf64-tboot10-tmp d2bad1a22`.
Using `./build.sh amd64 --drt --mem 0x230000000`. The result looks good. Most
of the `tpm12_*()` and `tpm20_*()` functions are stripped because not used, so
the SL size is well below 64K.

After some experiments, I realized that `-m elf_i386` on the command line is
needed (use `ld -m -v` for a list of supported values). Another way to solve
this problem is to add `OUTPUT_FORMAT(elf32-i386)`. (Currently there is only
`OUTPUT_ARCH(i386)`, but
<https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_mono/ld.html> says that
`OUTPUT_ARCH` is often unnecessary).

A possible related stackoverflow question:
<https://stackoverflow.com/questions/41930865/>

Also need to add `KEEP` command in ld scripts to prevent remove of necessary
symbols:
* bootloader: `.multiboot_header`
* secureloader: `.sl_header`
* runtime: `.s_rpb`
* runtime: `.xcph_table`

In `xmhf64-tboot10-tmp 5e9102d6c`, able to boot on Dell with the following
configurations:
```sh
./build.sh i386 --drt --dmap && gr
./build.sh amd64 --drt --dmap --mem 0x230000000 && gr
```

`xmhf64-tboot10-tmp` changes related to ld gc are ported to `xmhf64`. The
changes are ported to xmhf64:
* `xmhf64 fb0da8247`
	* `xmhf64-tboot10-tmp e3fdba0fe`
	* `xmhf64-tboot10-tmp 5e9102d6c`
* `xmhf64 7070d04ea`
	* `xmhf64-tboot10-tmp 3d839366e`
* `xmhf64 eb482ee2c`
	* `xmhf64-tboot10-tmp bfe759b26`
* `xmhf64 5185f1e6a`
	* `xmhf64-tboot10-tmp f2e4b6b31`
	* `xmhf64-tboot10-tmp b9065673d`
	* `xmhf64-tboot10-tmp 98ee51844`

TODO: (in `bug_097`) `TPM:CreatePrimary creating hierarchy handle = 40000007` takes a long time
TODO: fix TODOs in `tpm-x86.c`, also `xmhf-tpm-arch-x86.h`

