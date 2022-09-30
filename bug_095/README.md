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

