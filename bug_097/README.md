# Dell cannot run DRT

## Scope
* DRT enabled
* i386 and amd64
* `xmhf64 ac3f3a510`

## Behavior
When trying to run XMHF on Dell with DRT, the machine automatically resets
after seeing `GETSEC[SENTER]` in serial port. Log for amd64 (nested
virtualization branch) looks like `20221003220343`. Log for i386 looks like
`20221003221201`.

## Debugging

### Analyzing error code

From logs we can confirm that `TXT.ERRORCODE=80000005`. To get the error code
of a failure, use the error code printed during next time running TXT. Intel
TXT manual says this means:
* Type2 (bit 14 - 0): 5
* SoftwareSource (bit 15): 0 (Authenticated Code Module (ACM))
* Type1 (bit 29 - 16): 0
* Processor/External (bit 30): 0 (Error condition reported by processor)
* Valid/Invalid (bit 31): 1 (Valid error)

In `6th_7th_gen_i5_i7-SINIT_79.zip` downloaded from Intel, there is a file
`Skl_Errors_201505.xlsx`. Can look up error code using "Module, Class, Major,
Minor."

<https://twobit.org/2015/01/02/txt-and-tboot-on-the-ivb-nuc/> says that the
error code can be decoded as:
> * bit 31 - Valid
> * bit 30 - External
> * bits 29:25 - Reserved
> * bits 24:16 - Minor Error Code
> * bit 15 - SW Source
> * bits 14:10 - Major Error Code
> * bits 9:4 - Class Code
> * bits 3:0 - Module Type

This is using `SINIT_Errors.pdf` in `3rd-gen-i5-i7-sinit-67.zip`.

For some reason it is hard to find SDM for TXT in Intel's website in Google.
<https://lkml.org/lkml/2022/2/18/699> has a link to
<https://www.intel.com/content/dam/www/public/us/en/documents/guides/intel-txt-software-development-guide.pdf>
that works. The file name I get is
`315168_TXT_MLE_Development Guide_rev_017_3.pdf`.
* March 2022
* Intel(R) Trusted Execution Technology (Intel(R) TXT)
* Software Development Guide
* Measured Launch Environment Developer’s Guide
* Revision 017.3
* Document number: 315168-017

The newer version of SDM provides more granular decoding
* Type2 / Module Type (bit 3 - 0): 5
* Type2 / Class Code (bit 9 - 4): 0
* Type2 / Major Error Code (bit 14 - 10): 0
* Software Source (bit 15): 0 (Authenticated Code Module)
* Type1 / Minor Error Code (bit 27 - 16): 0
* Type1 / Reserved (bit 29 - 28): 0
* Processor/Software (bit 30): 0 (Error condition reported by processor)
  (see Table 24)
* Valid/Invalid (bit 31): 1 (Valid error)

Table 24 is a mapping from Error type code to Error condition. The Type has
value from 0 - 65535 (16 bits), but only 0 - 15 are defined. It is ambiguous
what the key is referring to, but I guess 5 may be the correct key.

Appendix H "ACM Error Codes" also contains some more examples. Actually,
Table 38. "TXT.ERRORCODE Register Format for CPU-initiated TXT-shutdown"
matches our situation. The key for Table 24 is Type2, which in our case is 5.

This means that Error condition is "Load memory type error in Authenticated
Code Execution Area" and Mnemonic is "#BadACMMType". By searching the mnemonic
in Intel v2, can see that the error reason is that memory type is not write
back:
> IF (ACRAM memory type != WB)
>     THEN TXT-SHUTDOWN(#BadACMMType);

### Check memory type

I noticed something strange with the AMT output. The printed MTRRs are
```
base	mask	type	v
0e0000	fe0000	00	1
0d0000	ff0000	00	1
0ce000	ffe000	00	1
0cd800	fff800	00	1
```

However, in Linux the MTRRs should be
```
0 base 00E0000000 mask 7FE0000000 uncachable
1 base 00D0000000 mask 7FF0000000 uncachable
2 base 00CE000000 mask 7FFE000000 uncachable
3 base 00CD800000 mask 7FFF800000 uncachable
```

Note that the first "7" in the mask column is truncated. In tboot 1.10.4, the
definition for `mtrr_physmask_t` is:
```c
typedef union {
    uint64_t    raw;
    struct {
        uint64_t reserved1 : 11;
        uint64_t v         : 1;      /* valid */
        uint64_t mask      : 52;     /* define as max width and mask w/ */
                                     /* MAXPHYADDR when using */
    };
} mtrr_physmask_t;
```

However, in XMHF it becomes
```c
typedef union {
    uint64_t    raw;
    struct {
        uint64_t reserved1 : 11;
        uint64_t v         : 1;      /* valid */
        /* TBD: the end of mask really depends on MAXPHYADDR, but since */
        /* the MTRRs are set for SINIT and it must be <4GB, can use 24b */
        uint64_t mask      : 24;
        uint64_t reserved2 : 28;
    } __attribute__((packed));
} mtrr_physmask_t;
```

That is, XMHF assumes the MAXPHYADDR is 36, but in Dell it becomes large than
that. This problem happens a long time ago. The earliest commit I can find is
`ecb78850c27524d3f4bdba09c8cbcee09c3f2102` in Jan 2011. I do not see such
change in tboot code.

This should be fixed because it is causing integer truncation. Fixed in
`xmhf64 6e5c5195c`. HP 2540p x86 XMHF with DRT works fine, but this does not
solve the problem on Dell.

After the above commit, CI fails because SL runs out of space in O3. In
`xmhf64 6e5c5195c..ca64abb5d`, remove functions that are not needed by SL in
`bplt-x86vmx-mtrrs.c`. GitHub Actions no longer fails.

In `xmhf64-dev 58c0697df`, print MTRRs after `set_mem_type()`. The printed
result on serial port is
```
LXY: base=0xccef0000
LXY: size=0x00020000
LXY: mem_type=0x00000006
LXY: print MTRR begin
mtrr_def_type: e = 1, fe = 0, type = 0
mtrrs:
		    base          mask      type  v
		00000000cced0 0000000ffffe0  06  1
		00000000d0000 0000007ff0000  00  0
		00000000ce000 0000007ffe000  00  0
		00000000cd800 0000007fff800  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
LXY: print MTRR end
```

Can see that the intention is to mark WB (type=6) from `0xccef0000` to
`0xccef0000 + 0x00020000 = 0xccf10000`. However, actual marked memory is from
`0xccec0000` to `0xccee0000`. Looks like the MTRR misses the ACRAM entirely.

```
base               ccef0000
size               00020000
begin              ccef0000
end                ccf10000
mtrr begin 00000000ccec0000
mtrr end   00000000ccee0000
mtrr base  00000000cced0000
mtrr mask  0000000ffffe0000
```

We can reproduce the computation logic in Linux using `mtrr1.c`. The problem is
in the implementation of `set_mem_type()`, which is not the same as tboot
1.10.4 code. The goal of this function is to mark only the memory region as WB,
while mark other memory region as another specified value. See Intel TXT doc
> Use MTRRs to map SINIT memory region as WB, all other regions
> are mapped to a value reported supportable by
> GETSEC[PARAMETERS]

In tboot 1.10.4 code, there are 2 while loops. Suppose the memory region to be
marked as WB starts with `begin` and ends with `end`. And suppose `max` is the
address that contains most number of suffix 0s between `start` and `end`. Then
* The first while loop `while (num_pages >= mtrr_s){` covers `start` to `max`.
* The second while loop `while ( num_pages > 0 ) {` covers `max` to `end`.

However, in XMHF code, only the second while loop exists. Probably the first
while loop is removed?

I copied `mtrr1.c` to `mtrr2.c`, and replaced `set_mem_type()` with the
implementation from tboot 1.10.4. After fixing compiler errors, I noticed that
tboot 1.10.4 code is very similar to original XMHF code. XMHF code just removed
the `while (num_pages >= mtrr_s){` while loop and some related code. See
`diff mtrr1.c mtrr2.c`.

Fixed in `xmhf64 e0277b23f`. After the fix, 2 MTRR entries are used to cover
the ACRAM.

```
mtrr_def_type: e = 1, fe = 0, type = 0
mtrrs:
		    base          mask      type  v
		00000000cced0 0000000fffff0  06  1
		00000000ccee0 0000000fffff0  06  1
		00000000ce000 0000007ffe000  00  0
		00000000cd800 0000007fff800  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
```

After the fix, Dell still resets after printing `executing GETSEC[SENTER]...`.
However, this should be considered progress because `TXT.ERRORCODE` changes
from `0x80000005` to `0xc0060000`.

### Unrecoverable machine check condition

Using Appendix H "ACM Error Codes", the new error code `0xc0060000` should be
decoded as:
* 31 Valid: 1
* 30 External: 1 - induced by external software
* 29:28 Type1 / Reserved: 0
* 27:16 Type1 / Minor Error Code: 6
* 15 SW source: 0 = ACM
* 14:10 Type2 / Major Error Code: 0
* 9:4 Type2 / Class Code: 0
* 3:0 Type2 / Module Type: 0 = BIOS ACM

This matches `Skl_Errors_201505.xlsx` with module=ANY, class=0
(`CLASS_ACM_PROGRESS`), major=0 (`ERR_OK`), minor=1-0xFFF,
minor_desc="Internal execution progress code". This does not look like an
error. Now it is better to check whether the first instruction of DRT has been
executed.

For some reason I cannot reproduce the `0xc0060000` error code. Now the new
error code I see becomes `TXT.ERRORCODE=8000000c`. This type indicates
0xC = Unrecoverable machine check condition, memonic `#UnrecovMCError`.

There are 2 occurrences of `TXT-SHUTDOWN(#UnrecovMCError);` in Intel i2.

Note that in TXT documentation, there is a check related to machine check on
bit `GETSEC[PARAMETERS](type=5)[6]`. For Dell this bit is 1, so machine check
registers do not need to be cleared.

From Intel i3 Table 9-2. "Variance of RESET Values in Selected Intel
Architecture Processors", `IA32_MCi_Status` are not modified across warm reset
for some CPU models. However, Dell's CPU should be `06_9EH`, to which this rule
does not apply. So Dell's behavior is undefined.

The relevant pseudo code snippet in Intel i2 is
```
FOR I = 0 to IA32_MCG_CAP.COUNT-1 DO
    IF IA32_MC[I]_STATUS = uncorrectable error
        THEN TXT-SHUTDOWN(#UnrecovMCError);
    FI;
OD;
IF (IA32_MCG_STATUS.MCIP=1) or (IERR pin is asserted)
    THEN TXT-SHUTDOWN(#UnrecovMCError);
```

I think `IA32_MC[I]_STATUS` likely caused the shutdown. If the CPU will
preserve the machine check banks, we can further debug the situation. This is
our best try because after TXT shutdown, the only way to let the machine
continue execution is to (warm) reset.

Next steps
* Print machine check banks
* Try running tboot, compare. I feel that #MC may be related to MTRR

### Double check MTRR

I realized that after the fix in `xmhf64 e0277b23f`, the MTRR result is still
wrong.

```
LXY: base=0xccef0000
LXY: size=0x00020000
LXY: mem_type=0x00000006
LXY: print MTRR begin
mtrr_def_type: e = 1, fe = 0, type = 0
mtrrs:
		    base          mask      type  v
		00000000cced0 0000000fffff0  06  1
		00000000ccee0 0000000fffff0  06  1
		00000000ce000 0000007ffe000  00  0
		00000000cd800 0000007fff800  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
		0000000000000 0000000000000  00  0
LXY: print MTRR end
```

<del>The MTRRs are covering 0xcced0000-0xccef0000, but we need to cover
0xccef0000-0xccf10000. The MTRR results are not the same as `mtrr2.c`. IIRC
the MTRRs are different between the input of `wrmsr64()` in `set_mem_type()`
and the printed value in `print_mtrrs()`.</del>

The previous print of base is wrong, because base changes in function
`set_mem_type()`. Should instead print the base in the beginning. See
`xmhf64-dev 2cf3416ca`. The correct values are:
```
LXY: base=0xcced0000
LXY: size=0x00020000
LXY: mem_type=0x00000006
```

This time MTRRs are covering the correct memory range.

### Check machine check banks

In `xmhf64-dev baa408dbb`, print machine check MSRs at the start of bootloader.
Then completely power off the machine, then run XMHF with DRT twice. The serial
output are in `20221005122111` and `20221005122114`. Can see that
`IA32_MC9_ADDR` and `IA32_MC9_MISC` changed. `IA32_MC9_ADDR` is `0xf000eec0`,
which is within reserved memory reported by E820. `IA32_MC9_MISC` is
`0x0000043020000086`. The lower bits indicate Address Mode = 010 (Physical
Address) and Recoverable Address LSB = 6.

I think the most important MSR, `IA32_MC9_STATUS`, is cleared on reset. So
debugging becomes hard. We can try looking into the address `0xf000eec0`, but
it may not be very helpful.

After trying a few more times, looks like this address is consistent.

### Running tboot

I am able to run tboot 1.9.12 on Dell, which is provided by apt-get in Debian.
Now need to be able to compile tboot from source in order to change. Maybe
`bug_046` can help.

To let tboot print to AMT serial port, use the same multiboot2 argument:
`serial=115200,8n1,0xf0a0`. Sample tboot output is in `20221005142345`.
Note that tboot prints
`TBOOT: acpi_table_mcfg @ 0xca50d490, .base_address = 0xf0000000`. This is
close to the `IA32_MC9_ADDR` address.

Next steps
* Discover what is in memory 0xf000eec0
	* MCFG is likely related to PCI, which I think is likely unrelated.
* Try running tboot, compare. I feel that #MC may be related to MTRR
	* I think MTRR is checked, probably something else is wrong
* Try to randomly insert WBINVD and maybe invalidate page table
	* Not likely, because TXT doc only requires WBINVD before and after MTRR.
	  Bootloader does not enable paging, and tboot 1.10.4 code does not flush
	  TLB.
* Try manually setting more / less memory as WB in MTRR
	* Not prioritized because reasoning about MTRR above

From comments in XMHF, looks like XMHF used tboot version `tboot-20101005`.
All versions can be downloaded in
<https://sourceforge.net/projects/tboot/files/tboot/>. Looks like this version
contains the MTRR bug I discovered earlier. This explains why XMHF contains 1
while loop but tboot 1.10.4 contains 2 while loops. Now we have better plans
* Try running older version of tboot, bisect to see which version breaks
* Compare different versions of tboot (e.g. `git diff`), can use unofficial Git
  mirror <https://github.com/tklengyel/tboot>
	* However, looks like this repository ends at May 2018, but there is active
	  development in 2022.
* Update XMHF with newer version of tboot

For now, downloaded 27 tar.gz versions from Source Forge.
```
tboot-1.10.5.tar.gz 	2022-03-09 	907.7 kB
tboot-1.10.4.tar.gz 	2022-02-24 	908.0 kB
tboot-1.10.3.tar.gz 	2021-12-10 	906.7 kB
tboot-1.10.2.tar.gz 	2021-06-14 	8.3 MB
tboot-1.10.1.tar.gz 	2021-03-30 	8.3 MB
tboot-1.10.0.tar.gz 	2020-12-11 	8.5 MB
tboot-1.9.12.tar.gz 	2020-04-29 	717.6 kB
tboot-1.9.11.tar.gz 	2019-12-12 	709.1 kB
tboot-1.9.10.tar.gz 	2019-04-24 	707.1 kB
tboot-1.9.9.tar.gz 	2018-11-30 	708.1 kB
tboot-1.9.8.tar.gz 	2018-10-18 	663.3 kB
tboot-1.9.7.tar.gz 	2018-09-03 	662.7 kB
tboot-1.9.6.tar.gz 	2017-07-12 	693.6 kB
tboot-1.9.5.tar.gz 	2016-12-20 	685.5 kB
tboot-1.9.4.tar.gz 	2016-08-06 	576.6 kB
tboot-1.8.3.tar.gz 	2015-05-07 	554.1 kB
tboot-1.8.2.tar.gz 	2014-07-28 	566.0 kB
tboot-1.8.1.tar.gz 	2014-05-16 	547.2 kB
tboot-1.8.0.tar.gz 	2014-01-30 	517.3 kB
tboot-1.7.4.tar.gz 	2013-08-12 	475.0 kB
tboot-1.7.3.tar.gz 	2012-12-28 	473.7 kB
tboot-1.7.2.tar.gz 	2012-09-29 	471.2 kB
tboot-1.7.1.tar.gz 	2012-04-27 	472.6 kB
tboot-1.7.0.tar.gz 	2012-01-26 	465.8 kB
tboot-1.6.tar.gz 	2011-08-04 	459.6 kB
tboot-20110714.tar.gz 	2011-07-26 	461.3 kB
tboot-20101005.tar.gz 	2010-11-02 	457.1 kB
```

### Compiling and running tboot

To compile tboot from source, use
```sh
cd tboot
make -j `nproc`
```

Then `tboot.gz` is generated. To install, put this file to `/boot/tboot.gz`,
replacing the previously installed one (using apt-get).

Running `tboot-1.10.5` (newest version) is easy, but running `tboot-20101005`
has some challenges
* Compiler warnings appear, need to remove `-Werror` in `Config.mk`
* multiboot2 is not supported, need to use multiboot. Change all `multiboot2`
  to `multiboot` and `module2` to `module`.

```
results/20221007133734-tboot-1.10.5		good (can boot Linux)
tboot-1.10.4
tboot-1.10.3
tboot-1.10.2
tboot-1.10.1
tboot-1.10.0
tboot-1.9.12
tboot-1.9.11
tboot-1.9.10
results/20221007135745-tboot-1.9.9		good (can boot Linux)
tboot-1.9.8
tboot-1.9.7
tboot-1.9.6
tboot-1.9.5
tboot-1.9.4
results/20221007140124-tboot-1.8.3		TXT.ERRORCODE: 0xc0060000
results/20221007141811-tboot-1.8.2		Unable to test: TPM not ready
tboot-1.8.1
results/20221007141345-tboot-1.8.0		Unable to test: TPM not ready
results/20221007141025-tboot-1.7.4		Unable to test: TPM not ready
tboot-1.7.3
tboot-1.7.2
tboot-1.7.1
tboot-1.7.0
tboot-1.6
results/20221007135309-tboot-20101005	Unable to test: TPM not ready
```

Notes:
* tboot-1.10.5: good
* tboot-20101005: need multiboot, bug before `GETSEC[SENTER]` is attempted
* tboot-1.9.9: good
* tboot-1.8.3: another bug, but happens after `GETSEC[SENTER]`
* tboot-1.7.4: need multiboot, same as tboot-20101005
* tboot-1.8.0: have multiboot2, bug same as tboot-20101005 and tboot-1.7.4
* tboot-1.8.2: same as tboot-1.8.0

The above experiments indicate that the bug should be fixed between
`tboot-20101005` and `tboot-1.8.3`. However older versions cannot be
reproduced, so we try to manually review the code

* bca156d tboot-1.8.3.tar.gz
* c436549 tboot-1.8.2.tar.gz
* f8eaf0e tboot-1.8.1.tar.gz
* 87f8cfa tboot-1.8.0.tar.gz
* a81d112 tboot-1.7.4.tar.gz
* 29c58cb tboot-1.7.3.tar.gz
* 85a1945 tboot-1.7.2.tar.gz
* 2f4f636 tboot-1.7.1.tar.gz
* 630b424 tboot-1.7.0.tar.gz
* 3197047 tboot-1.6.tar.gz
	* Bug in `mtrrs.c` fixed
* a5aab18 tboot-20101005.tar.gz
	* Initial commit, no review

It becomes a pain to review all the code because there are too much change.
However, in the mean time I noticed that XMHF prints two warnings (e.g.
`20221003221201`)
```
unsupported BIOS data version (6)
SINIT's os_sinit_data version unsupported (7)
```

However, Intel TXT manual says the data structures should be backward
compatible, so likely not related to the error.

Current ideas:
* Print and compare data structures between XMHF and tboot
* Replace XMHF code with new tboot version code
* Review XMHF code, check with TXT documentation
* Manually implement TXT documentation
* Make tboot < 1.8.3 reproducible to bisect more

### How XMHF adapts tboot code

Search for the following to find files that are likely adapted from Intel
* "Intel Corporation" (comment for copyright)
* `__DRT__`
* `$(DRT)` in Makefile
* `tboot`

Files in XMHF:
* `xmhf/src/libbaremetal/libtpm/include/tpm.h`
	* line 108 - 330 are from `tboot/include/tpm.h`
* `xmhf/src/libbaremetal/libtpm/tpm.c`
	* Selected functions from `tboot/common/tpm.c`
* `xmhf/src/libbaremetal/libtpm/tpm_extra.c`
	* Rest of `tboot/common/tpm.c`, looks like not used
* `xmhf/src/libbaremetal/libxmhfutil/cmdline.c`
	* `get_option_val()` and `cmdline_parse()` from `tboot/common/cmdline.c`
* `xmhf/src/libbaremetal/libxmhfutil/include/cmdline.h`
	* From `tboot/include/cmdline.h`
* `xmhf/src/xmhf-core/include/arch/x86/_cmdline.h`
	* From `tboot/include/cmdline.h`
* `xmhf/src/xmhf-core/include/arch/x86/_com.h`
	* From `tboot/include/com.h`
* `xmhf/src/xmhf-core/include/arch/x86/_error.h`
	* From `tboot/include/misc.h`
* `xmhf/src/xmhf-core/include/arch/x86/_msr.h`
	* Not related
* `xmhf/src/xmhf-core/include/arch/x86/_txt_acmod.h`
	* From `tboot/include/txt/acmod.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt_config_regs.h`
	* From `tboot/include/txt/config_regs.h` and `tboot/include/txt/errorcode.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt.h`
	* Not related
* `xmhf/src/xmhf-core/include/arch/x86/_txt_hash.h`
	* From `include/hash.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt_heap.h`
	* From `tboot/include/txt/heap.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt_mle.h`
	* From `include/mle.h` and `include/uuid.h` and `include/tb_error.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt_mtrrs.h`
	* From `tboot/include/txt/mtrrs.h`
* `xmhf/src/xmhf-core/include/arch/x86/_txt_smx.h`
	* From `tboot/include/txt/smx.h`
* `xmhf/src/xmhf-core/include/arch/x86/xmhf-tpm-arch-x86.h`
	* Not related
* `xmhf/src/xmhf-core/xmhf-bootloader/cmdline.c`
	* From `tboot/common/cmdline.c`
* `xmhf/src/xmhf-core/xmhf-bootloader/init.c`
	* `txt_supports_txt()`: based on `tboot/txt/verify.c` function
	  `supports_vmx()`, `supports_smx()`, ...
	* `txt_verify_platform()`: based on `tboot/txt/verify.c`
	* `txt_status_regs()`: based on `tboot/txt/errors.c`
* `xmhf/src/xmhf-core/xmhf-bootloader/txt_acmod.c`
	* From `tboot/txt/acmod.c`
* `xmhf/src/xmhf-core/xmhf-bootloader/txt.c`
	* From `tboot/txt/txt.c`
* `xmhf/src/xmhf-core/xmhf-bootloader/txt_hash.c`
	* From `tboot/common/hash.c`
* `xmhf/src/xmhf-core/xmhf-bootloader/txt_heap.c`
	* From `tboot/txt/heap.c`
* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx.c`
	* Call to `restore_mtrrs()` etc: from `tboot/txt/txt.c` function
	  `txt_post_launch()`
* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-bootloader.c`
	* Some functions in `tboot/txt/mtrrs.c`
* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-common.c`
	* Other functions in `tboot/txt/mtrrs.c`
* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-smp.c`
	* `__DRT__`: wake up APs, from `tboot/txt/txt.c` function `txt_wakeup_cpus()`
* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/runtime.c`
	* Not related (`vmx_eap_zap()`)
* `xmhf/src/xmhf-core/xmhf-secureloader/arch/x86/sl-x86.c`
	* Call to `restore_mtrrs()` etc: from `tboot/txt/txt.c` function
	  `txt_post_launch()`
* `xmhf/src/xmhf-core/xmhf-secureloader/sl.c`
	* Not related

Modifications to be noticed
* Replace the following with `printf(`
	* tboot-20101005: replace `printk(`
	* tboot-1.10.5: replace re `printk\(TBOOT_(NONE|ERR|WARN|INFO|DETA|ALL)`
* Change in symbol names
	* `wrmsr -> wrmsr64`
	* `rdmsr -> rdmsr64`
	* `read_eflags -> get_eflags` (manual, because how to call)
	* `write_eflags -> set_eflags`
	* `PAGE_SIZE -> PAGE_SIZE_4K`
	* `PAGE_SHIFT -> PAGE_SHIFT_4K`
	* `PAGE_UP -> PAGE_ALIGN_UP_4K` (manual, because type matters)
	* `tb_strcmp -> strcmp`
	* `tb_strlen -> strlen`
	* `tb_strchr -> strchr`
	* `tb_strncmp -> strncmp`
	* `tb_strncpy -> strncpy`
	* `__text -> (dropped)` (manual)
	* `tb_memcpy -> memcpy`
	* `tb_memcmp -> memcmp`
	* `tb_memset -> memset`
	* `__packed -> __attribute__((packed))` (manual)
	* `%Lx -> %llx` in printf (manual)
	* `MSR_IA32_PLATFORM_ID -> IA32_PLATFORM_ID`
	* `COMPILE_TIME_ASSERT -> _Static_assert`
	* `__data -> (dropped)` (manual)
	* `PAGE_MASK -> PAGE_MASK_4K`
	* `X86_EFLAGS_VM -> EFLAGS_VM`
* Functions not defined in XMHF
	* `ARRAY_SIZE`
	* `cpu_relax`
* Remove spaces at end of line
* XMHF has "ISO C90 forbids mixed declarations and code", but tboot does not
	* To workaround, can remove `-Wdeclaration-after-statement` in Makefile
* Header file names need to be changed
	* Usually XMHF combines header files to `<xmhf.h>`
* Check changes to support x64 XMHF
* Remove ifdefs on `IS_INCLUDED`
* For header files, add ifdefs on `__ASSEMBLY__`

```sh
tboot-sed () {

sed \
 -e 's/\bprintk(\(TBOOT_\(NONE\|ERR\|WARN\|INFO\|DETA\|ALL\)\b\)\?/printf(/g' \
 -e 's/\bwrmsr\b/wrmsr64/g' \
 -e 's/\brdmsr\b/rdmsr64/g' \
 -e 's/\bPAGE_SIZE\b/PAGE_SIZE_4K/g' \
 -e 's/\bPAGE_SHIFT\b/PAGE_SHIFT_4K/g' \
 -e 's/\bread_eflags\b/get_eflags/g' \
 -e 's/\bwrite_eflags\b/set_eflags/g' \
 -e 's/[[:blank:]]*$//' \
 -e 's/\btb_strcmp\b/strcmp/g' \
 -e 's/\btb_strlen\b/strlen/g' \
 -e 's/\btb_strchr\b/strchr/g' \
 -e 's/\btb_strncmp\b/strncmp/g' \
 -e 's/\btb_strncpy\b/strncpy/g' \
 -e 's/\btb_memcpy\b/memcpy/g' \
 -e 's/\btb_memcmp\b/memcmp/g' \
 -e 's/\btb_memset\b/memset/g' \
 -e 's/\bMSR_IA32_PLATFORM_ID\b/IA32_PLATFORM_ID/g' \
 -e 's/\bCOMPILE_TIME_ASSERT\b/_Static_assert/g' \
 -e 's/\bPAGE_MASK\b/PAGE_MASK_4K/g' \
 -e 's/\bX86_EFLAGS_VM\b/EFLAGS_VM/g' \
 "$@"

}
```

Start a new branch called `xmhf64-tboot10` to perform the change. Based on
`xmhf64 c426b0e0e`.

Progress:
* `c426b0e0e..fe67bc73e`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_mtrrs.h`
		* From `tboot/include/txt/mtrrs.h`
	* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-bootloader.c`
		* Some functions in `tboot/txt/mtrrs.c`
	* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-common.c`
		* Other functions in `tboot/txt/mtrrs.c`
* `fe67bc73e..156acde75`
	* `xmhf/src/libbaremetal/libxmhfutil/cmdline.c`
		* `get_option_val()` and `cmdline_parse()` from `tboot/common/cmdline.c`
	* `xmhf/src/libbaremetal/libxmhfutil/include/cmdline.h`
		* From `tboot/include/cmdline.h`
	* `xmhf/src/xmhf-core/include/arch/x86/_cmdline.h`
		* From `tboot/include/cmdline.h`
	* `xmhf/src/xmhf-core/include/arch/x86/_com.h`
		* From `tboot/include/com.h`
	* `xmhf/src/xmhf-core/xmhf-bootloader/cmdline.c`
		* From `tboot/common/cmdline.c`
* `156acde75..eac573336`
	* `xmhf/src/xmhf-core/include/arch/x86/_error.h`
		* From `tboot/include/misc.h`
* `eac573336..c97a02172`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_hash.h`
		* From `include/hash.h`
	* `xmhf/src/xmhf-core/xmhf-bootloader/txt_hash.c`
		* From `tboot/common/hash.c`
* `c97a02172..558ae22df`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_mle.h`
		* From `include/mle.h` and `include/uuid.h` and `include/tb_error.h`
* `558ae22df..daabfe306`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_heap.h`
		* From `tboot/include/txt/heap.h`
	* `xmhf/src/xmhf-core/xmhf-bootloader/txt_heap.c`
		* From `tboot/txt/heap.c`
* `daabfe306..61180d550`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_config_regs.h`
		* From `tboot/include/txt/config_regs.h` and `tboot/include/txt/errorcode.h`
* `61180d550..220a785ae`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_smx.h`
		* From `tboot/include/txt/smx.h`
* `220a785ae..c136fc6a1`
	* `xmhf/src/xmhf-core/include/arch/x86/_txt_acmod.h`
		* From `tboot/include/txt/acmod.h`
	* `xmhf/src/xmhf-core/xmhf-bootloader/txt_acmod.c`
		* From `tboot/txt/acmod.c`
	* Partial work (cherry-picked): 5ae921eb0 (384f8b507, 
	  <del>xmhf64-tboot10-tmp</del>)
* `c136fc6a1..9cf234895`
	* TPM code, see `tpm_symbols.txt` for list of symbols in tboot's `tpm.c`
	  before change.
	* tboot has different function pointers for TPM 1.2 and TPM 2.0. See
	  `tpm_12.c` and `tpm_20.c`. This time only support TPM 1.2.
	* `xmhf/src/libbaremetal/libtpm/include/tpm.h`
		* line 108 - 330 are from `tboot/include/tpm.h`
	* `xmhf/src/libbaremetal/libtpm/tpm.c`
		* Selected functions from `tboot/common/tpm.c`
	* `xmhf/src/libbaremetal/libtpm/tpm_extra.c`
		* Rest of `tboot/common/tpm.c`, looks like not used
	* TrustVisor uses 2 tpm interfaces provided by XMHF.
		* These should require `__DRT__`
		* `xmhf_tpm_open_locality`
			* `xmhf_tpm_arch_open_locality`
				* `xmhf_tpm_arch_x86vmx_open_locality`
					* Similar to tboot `find_platform_sinit_module()` + 
					  `write_priv_config_reg(TXTCR_CMD_OPEN_LOCALITY1, 0x01);`
						* `find_platform_sinit_module()` is probably unneeded
			* `xmhf_tpm_is_tpm_ready`
				* Tboot `is_tpm_ready()`
					* Probably not needed, or only `tpm_validate_locality()`
					  needed
		* `xmhf_tpm_deactivate_all_localities`
			* Not a function in tboot, but see 4 occorrences of
			  `write_tpm_reg(locality, TPM_REG_ACCESS, &reg_acc);`
	* Start seeing secureloader bloat reported by GitHub Actions
* `9cf234895..19e0c15ce`
	* Added TPM 2.0 code (`tpm_20.c`), secureloader bloat is severe, used
	  insecure workaround.
	* Increased SL low memory from 64 KiB to 128 KiB
* `19e0c15ce..f7a816cb4`
	* `xmhf/src/xmhf-core/xmhf-bootloader/init.c`
		* `txt_supports_txt()`: based on `tboot/txt/verify.c` function
		  `supports_vmx()`, `supports_smx()`, ...
		* `txt_verify_platform()`: based on `tboot/txt/verify.c`
		* `txt_status_regs()`: based on `tboot/txt/errors.c`,
		  original name `txt_display_errors()`
* `f7a816cb4..e564f2510`
	* `xmhf/src/xmhf-core/xmhf-bootloader/txt.c`
		* From `tboot/txt/txt.c`
		* Change content of `g_mle_hdr`, `print_file_info`
		* `find_platform_sinit_module() -> check_sinit_module()`
		* `find_lcp_module() -> dropped`
		* `txt_wakeup_cpus() -> dropped`
		* `txt_is_launched() -> moved`
		* `txt_s3_launch_environment() -> dropped`
		* `txt_post_launch() -> dropped`
		* `txt_cpu_wakeup() -> dropped`
		* `txt_protect_mem_regions() -> dropped`
		* `txt_shutdown() -> dropped`
		* `txt_is_powercycle_required() -> dropped`
* (no commit)
	* Not a lot of changes across tboot versions, also happens after SENTER
	* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx.c`
		* Call to `restore_mtrrs()` etc: from `tboot/txt/txt.c` function
		  `txt_post_launch()`
	* `xmhf/src/xmhf-core/xmhf-secureloader/arch/x86/sl-x86.c`
		* Call to `restore_mtrrs()` etc: from `tboot/txt/txt.c` function
		  `txt_post_launch()`
	* `xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-smp.c`
		* `__DRT__`: wake up APs, from `tboot/txt/txt.c` function `txt_wakeup_cpus()`
* `e564f2510..26666e75a`
	* See TXT.ERRORCODE = 0xc00c0491, from `Skl_Errors_201505.xlsx` the erorr
	  is "Computed heap data table size mismatches value in header". Looks like
	  need to fix some unimplemented code in `txt_heap.c`.
	* At this point, hp DRT works well. Now we try to bisect and find out the
	  change. I think it is impractical to port all tboot updates to XMHF at
	  this point.

Future work
* Use branch `xmhf64-tboot10`
* Secure loader bloat since `9cf234895`
* `txt_heap.c`: `TODO: Hardcoding get_evtlog_type() to EVTLOG_TPM2_TCG.`
* acmod: `verify_IA32_se_svn_status` was blocked on `tpm`

### Long term considerations

The bad news is that Wikipedia says "TPM 2.0 is not backward compatible with
TPM 1.2". In tboot code, TPM 1.2 and TPM 2.0 have different behavior, and are
managed using function pointers. See `tpm_12.c` and `tpm_20.c` (these files
are created since tboot 1.8.0)
* Ref: <https://en.wikipedia.org/wiki/Trusted_Platform_Module>

Also, for example, Dell says Windows 7 supports TPM 1.2, but not TPM 2.0. This
means XMHF needs to add a lot of code to support TPM 2.0.
<https://www.dell.com/support/kbdoc/en-us/000131631/tpm-1-2-vs-2-0-features>

Also, tboot updates reflect bugs in tboot that also show up in XMHF. First is
the MTRR problem discovered in this bug. There is also the NMI bug fixed in
`txt_wakeup_cpus()`: (manual diff)
```diff
<399      /* enable SMIs on BSP before waking APs (which will enable them on APs)
<400         because some SMM may take immediate SMI and hang if AP gets in first */
<401      printk("enabling SMIs on BSP\n");
<402      __getsec_smctrl();
>742      /* enable SMIs on BSP before waking APs (which will enable them on APs)
>743         because some SMM may take immediate SMI and hang if AP gets in first */
>744      printk(TBOOT_DETA"enabling SMIs and NMI on BSP\n");
>745      __getsec_smctrl();
<746      __enable_nmi();
```

### Bisecting tboot update

We bisect the changes made in `xmhf64-tboot10 c426b0e0e..26666e75a` to find out
what fixes the TXT.ERRORCODE 0x8000000c bug. List of commits is:
* 26666e75a `Workaround txt_heap.c by assuming EVTLOG_TPM2_TCG`
	* Good, serial `20221013231549`
* e564f2510 `Replace %Lx with %llx in printf`
* e25add70b `Move declarations before statements in txt.c`
* b1e3801ba `Add argument sinit for get_evtlog_type().`
* f36c8840d `Copy txt.c from tboot to XMHF`
* f7a816cb4 `Update tboot functions in init.c`
	* Apply patch: `%Lx -> %llx` in `init.c`
	* Bad `TXT.ERRORCODE: 0x8000000c`, serial `20221013232637` (two times)
* 19e0c15ce `Fix constants for secureloader sizes`
* 54c53c8f7 `Move declarations before statements in tpm_20.c`
* 274569141 `Copy tpm_20.c from tboot to XMHF (insecure)`
* 9cf234895 `Move declarations before statements in tpm.c and tpm_12.c`
* cf036dd28 `Copy tpm.c, tpm_12.c, and tpm.h from tboot to XMHF`
* 38e516db5 `Fix bug in 848b04a98d0ce7bab5bc6268601ac60836b883ed`
* 848b04a98 `Remove xmhf_tpm_prepare_tpm() (unused function)`
* 21bad9397 `Make txt_is_launched non-static`
* c136fc6a1 `Replace %Lx with %llx in printf`
* 688f2c29e `Update comments for txt_acmod.c`
* d59c059a9 `Move declarations before statements in txt_acmod.c`
* 29124156a `Copy txt_acmod.c and txt_acmod.h from tboot to XMHF`
* 220a785ae `Copy smx.h and txt.h from tboot to XMHF (file _txt_smx.h)`
* 61180d550 `Copy config_regs.h and errorcode.h from tboot to XMHF (file _txt_config_regs.h)`
* daabfe306 `Replace %Lx with %llx in printf`
* 95c4a5dd8 `Move declarations before statements in txt_heap.c`
* 83e301fe1 `Copy heap.c and heap.h from tboot to XMHF`
* 558ae22df `Copy mle.h, uuid.h, and tb_error.h from tboot to XMHF (file _txt_mle.h)`
* c97a02172 `Rename SHA_DIGEST_LENGTH to SHA1_DIGEST_LENGTH`
* 33db641d4 `Copy hash.c and hash.h from tboot to XMHF`
* eac573336 `Add comment about functions taken from tboot-1.10.5/include/misc.h`
* 156acde75 `Move declarations before statements in cmdline.c`
* 50fe6bde2 `Copy com.h, cmdline.c, and cmdline.h from tboot to XMHF`
* fe67bc73e `Add comments about how mtrrs.c is splitted`
* 2b1007465 `Move declarations before statements`
* 79d673295 `Copy mtrrs.c and mtrrs.h from tboot to XMHF`
* c426b0e0e

Between `f7a816cb4..26666e75a`, most of the changes happen in `txt.c`. So we
start a new branch from `f7a816cb4` and try to approach `26666e75a`.

The changes are between git versions `f7a816cb4..f2fff7342`. `f2fff7342` has
the same content as `26666e75a`. The commit `23ac4a5a4` in between fixes the
bug. I realize that the OS to SINIT Data Table version is 7, but on old tboot /
XMHF the version is 5. Updating the version fixes the TXT.ERRORCODE 0x8000000c
bug.

The version is detected with the following call stack
```
init_txt_heap()
	get_supported_os_sinit_data_ver()
		get_acmod_info_table()
			Return acm_info_table_t type
			See 315168-017 Table 10. Chipset AC Module Information Table
		Return info_table->os_sinit_data_ver (OsSinitDataVer, offset 24)
	Get version = 7
```

The Intel documentation says that OsSinitDataVer is
> Indicates the maximum version number of the OS
> to SINIT data structure that this module supports.
> It is assumed that the module is backward
> compatible with previous versions.

So ideally this bug should not happen, since it should be backward compatible.

### Bisecting OS to SINIT Data Table

Conveniently, `print_os_sinit_data()` already dumps the OS to SINIT data in
serial output. Compare `20221013232637` (bad) and `20221013231549` (good) shows

```diff
--- /tmp/bad	2022-10-14 13:47:30.747965869 -0400
+++ /tmp/good	2022-10-14 13:47:39.310127219 -0400
@@ -1,24 +1,32 @@
-os_sinit_data (@0xccf3517e, 64):
-	 version: 5
-	 flags: 0
+os_sinit_data (@0xccf3517e, 88):
+	 version: 7
+	 flags: 1
 	 mle_ptab: 0x10000000
 	 mle_size: 0x10000 (%Lu)
 	 mle_hdr_base: 0x0
 	 vtd_pmr_lo_base: 0x10000000
 	 vtd_pmr_lo_size: 0xd200000
 	 vtd_pmr_hi_base: 0x0
 	 vtd_pmr_hi_size: 0x0
 	 lcp_po_base: 0x0
 	 lcp_po_size: 0x0 (%Lu)
-	 capabilities: 0x00000622
+	 capabilities: 0x00000602
 	     rlp_wake_getsec: 0
 	     rlp_wake_monitor: 1
 	     ecx_pgtbl: 0
 	     stm: 0
 	     pcr_map_no_legacy: 0
-	     pcr_map_da: 1
+	     pcr_map_da: 0
 	     platform_type: 0
 	     max_phy_addr: 0
 	     tcg_event_log_format: 1
 	     cbnt_supported: 1
 	 efi_rsdt_ptr: 0x0
+	 ext_data_elts[]:
+	 TCG EVENT_LOG_PTR:
+		       type: 8
+		       size: 28
+	 TCG Event Log Descrption:
+	     allcoated_event_container_size: 8192
+	                       EventsOffset: [0,0]
+			 No Event Log found.
```

Now we modify good to try to approach bad.
1. Change `flag` from 1 to 0: still good
2. Also change `pcr_map_da` from 0 to 1: still good
3. Also change version from 7 to 5: bad, 0x8000000c
	* Need to force `*size = 0x64` after calling `calc_os_sinit_data_size()`
4. Perform 1 and 2, then change version from 7 to 6: bad, 0x8000000c

Looks like the problem is the `os_sinit_data` version. The version field
description says:
> Version number of the OsSinitData table.
> Current values are 6 for TPM 1.2 family and 7
> for TPM 2.0 family. No other values are
> supported. This value is incremented for any
> change to the definition of this table. Future
> versions will always be backwards compatible
> with previous versions (new fields will be
> added at the end).

So I gues the machine check error happens because SINIT is expecting TPM 1.2,
but TPM 2.0 is present. It is very hard to debug, though. We should remove the
hard-coded the version in XMHF code to solve this problem.

Further experiment (set `*size` to `0x68`, remove call to
`init_os_sinit_ext_data()`) shows that when TPM event logs are not present in
ExtDataElements, `GETSEC[SENTER]` will error with TXT.ERRORCODE 0xc00c0491.
Looks like TPM event logs are required for newer SINIT versions:
* C.4.1 `HEAP_TPM_EVENT_LOG_ELEMENT`:
  "SINIT in TPM 1.2 mode will require HEAP_TPM_EVENT_LOG_ELEMENT to be present
  since event log is required for attestation."
* C.4.2 `HEAP_EVENT_LOG_POINTER_ELEMENT2_1`:
  "SINIT in TPM2.0 mode will require HEAP_EVENT_LOG_POINTER_ELEMENT2_1 to be
  present since event log is required for attestation and many of the
  SinitMleData table fields carrying similar data are removed in TPM2.0 mode -
  see Table 30"

So the changes we need to make to XMHF are:
* Add version detection, force version to be within 5 - 7
* Update the `flags` field if needed
* Update the `pcr_map_da` field if needed
* Add TPM event log support (probably just allocate some memory, ignore content)

The changes are carried out in `xmhf64-dev 0982aec11..a4891901d`. Committed to
`xmhf64 c426b0e0e..8e7dd1c8c`. Now `GETSEC[SENTER]` works on Dell and
TrustVisor halts due to access to TPM 2.0 failure. HP 2540p is also good (no
regression).

### Typo in SDM

Found two typos in SDM 315168-017
* Intel(R) Trusted Execution Technology (Intel(R) TXT)
* Software Development Guide
* Measured Launch Environment Developer's Guide
* March 2022
* Revision 017.3
* Document number: 315168-017

Problems
1. Document Number for page 2 and beyond is "Document Number: 315168-170"
2. In "Table 29. OS to SINIT Data Table", it says "EFI RSDP Pointer" exists
   when "Versions >= 6". Should be "Versions >= 5". (see old tboot versions
   and old SDM (315168-006))

Reported in
<https://community.intel.com/t5/Intel-Trusted-Execution/Typos-in-TXT-development-guide-315168-017/m-p/1422314#M16>

### TrustVisor access TPM problem

TODO
TODO: document the update with new version of tboot

