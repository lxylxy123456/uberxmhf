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

The MTRRs are covering 0xcced0000-0xccef0000, but we need to cover
0xccef0000-0xccf10000. The MTRR results are not the same as `mtrr2.c`. IIRC
the MTRRs are different between the input of `wrmsr64()` in `set_mem_type()`
and the printed value in `print_mtrrs()`.

TODO: looks like the MTRRs are still wrong

TODO: print machine check banks
TODO: try running tboot, compare. I feel that #MC may be related to MTRR
TODO: TXT.ERRORCODE=8000000c

