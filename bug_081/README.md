# XMHF regression on 2540P

## Scope

* i386 XMHF, not tested on amd64
* HP 2540P, not on QEMU
* Git `b32661c70..d8f2220a6`

## Behavior

Miao reported that recently xmhf64 become un-bootable on 2540p. The commit
should be after PR `#8` and before `d8f2220a6`. So it should be
`b32661c70..d8f2220a6`.

The log is like 
```
BSP rallying APs...
BSP(0x00): My ESP is 0x25551000
APs all awake...Setting them free...
AP(0x01): My ESP is 0x25555000, proceeding...
[00]: unhandled exception 13 (0xd), halting!
[00]: error code: 0x00000000
[00]: state dump follows...
[00] CS:EIP 0x0008:0x1021353e with EFLAGS=0x00010006
[00]: VCPU at 0x1024ab60
AP(0x05): My ESP is 0x2555d000, proceeding...
AP(0x04): My ESP is 0x25559000, proceeding...
[00] EAX=0x00000491 EBX=0x1024ab60 ECX=0x00000491 EDX=0x00da0400
[00] ESI=0x1024ab60 EDI=0x1024e49c EBP=0x25550ad4 ESP=0x25550aa8
[00] CS=0x0008, DS=0x0010, ES=0x0010, SS=0x0010
[00] FS=0x0010, GS=0x0010
[00] TR=0x0018
```

## Debugging

Since currently I do not have access to HP 2540P machine. So the plan is to
compile XMHF and send the binary to Miao. Then after seeing the log I can
translate the addresses to symbols.

From the log, it looks like the problem happens on all CPUs soon after calling
`xmhf_runtime_main()`. However we need symbols to get more insight.

The compile commands are (some commands are used with `linksrc.sh`)

```sh
cd .. && umount build32 && linksrc build32 && cd build32
./autogen.sh
./configure '--with-approot=hypapps/trustvisor' '--enable-debug-symbols' '--disable-drt' '--enable-dmap' '--enable-debug-qemu' '--enable-update-intel-ucode'
make -j 4
# gcc (GCC) 11.3.1 20220421 (Red Hat 11.3.1-2)
find -type l -delete
7za a /tmp/d8f2220a6-build32.7z * -mx=1
```

For git `d8f2220a6`, the serial output is `202206081629`. We can use
`read_stack32.py` to read its stack (`read_stack.py` is for 64-bits only). The
output of
```sh
python3 ../notes/bug_081/read_stack32.py ../notes/bug_081/results/202206081629 xmhf/src/xmhf-core/xmhf-runtime/runtime.exe | grep '^\[00\]'
```
is in `202206081629read_stack32.txt`. However, some symbols are incorrect
because `nm` does not show `static inline` symbols. The manually modified file
is in `202206081629read_stack32_modify.txt`.

The call stack is
```
_ap_pmode_entry_with_paging
 xmhf_baseplatform_arch_x86_smpinitialize_commonstart
  xmhf_runtime_main
   xmhf_partition_initializemonitor
    xmhf_partition_arch_initializemonitor
     xmhf_partition_arch_x86vmx_initializemonitor
      _vmx_initVT
       rdmsr64
        RDMSR (ecx=0x00000491)
```

This bug happens due to support to the following fields.
```
#define IA32_VMX_EPT_VPID_CAP_MSR         0x48C
#define IA32_VMX_TRUE_PINBASED_CTLS_MSR   0x48D
#define IA32_VMX_TRUE_PROCBASED_CTLS_MSR  0x48E
#define IA32_VMX_TRUE_EXIT_CTLS_MSR       0x48F
#define IA32_VMX_TRUE_ENTRY_CTLS_MSR      0x490
#define IA32_VMX_VMFUNC_MSR               0x491
```

Looks like `IA32_VMX_VMFUNC_MSR` is not supported in the machine. However, this
is strange because I should be correctly using Intel v4 to determine whether
the MSR exists. In the SDM, MSR 0x490 and 0x491 should have the same condition
of existence. So if there is the error, it should happen earlier.
> 48DH `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`
> 48EH `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`
> 48FH `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`
> 490H `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`
> 491H `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`

As a workaround, we just need to remove support for MSR 0x491. See git
`195973e9a..f326512f4`.

To debug this problem, use git `5067ce7ec` in `xmhf64-dev` branch. This should
print a lot of useful information.

The output is

```
CPU(0x00): RDMSR(0x00000480) = 0x00da04000000000f
CPU(0x00): RDMSR(0x00000481) = 0x0000007f00000016
CPU(0x00): RDMSR(0x00000482) = 0xfff9fffe0401e172
CPU(0x00): RDMSR(0x00000483) = 0x007fffff00036dff
CPU(0x00): RDMSR(0x00000484) = 0x0000ffff000011ff
CPU(0x00): RDMSR(0x00000485) = 0x00000000000401e7
CPU(0x00): RDMSR(0x00000486) = 0x0000000080000021
CPU(0x00): RDMSR(0x00000487) = 0x00000000ffffffff
CPU(0x00): RDMSR(0x00000488) = 0x0000000000002000
CPU(0x00): RDMSR(0x00000489) = 0x00000000000267ff
CPU(0x00): RDMSR(0x0000048a) = 0x000000000000002a
CPU(0x00): RDMSR(0x0000048b) = 0x000000ff00000000
CPU(0x00): RDMSR(0x0000048c) = 0x00000f0106114141
CPU(0x00): RDMSR(0x0000048d) = 0x0000007f00000016
CPU(0x00): RDMSR(0x0000048e) = 0xfff9fffe04006172
CPU(0x00): RDMSR(0x0000048f) = 0x007fffff00036dfb
CPU(0x00): RDMSR(0x00000490) = 0x0000ffff000011fb
CPU(0x00): RDMSR(0x00000491) = unavailable
CPU(0x00): RDMSR(0x00000492) = unavailable
CPU(0x00): RDMSR(0x00000493) = unavailable
```

Looks like this is the SDM's fault. On the CPU, there is
`CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55]`, but MSR 491H does not exist.

If this problem is fixed in the future, need to consider reverting `f326512f4`.

### SDM typo

Looks like this is really a typo in Intel. Intel v3 "A.11 VM FUNCTIONS" says
> The `IA32_VMX_VMFUNC` MSR exists only on processors that support the 1-setting
> of the "activate secondary
> controls" VM-execution control (only if bit 63 of the
> `IA32_VMX_PROCBASED_CTLS` MSR is 1) and the 1-setting of
> the "enable VM functions" secondary processor-based VM-execution control
> (only if bit 45 of the `IA32_VMX_PROCBASED_CTLS2` MSR is 1).

However, Intel v4 says
> `If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_BASIC[55] )`

The correct comment is
`If( CPUID.01H:ECX.[5] = 1 && IA32_VMX_PROCBASED_CTLS[63] && IA32_VMX_PROCBASED_CTLS2[45] )`

Reported in
<https://community.intel.com/t5/Processors/Typo-in-SDM-volume-4/m-p/1391506>

In `f326512f4..f24d67709`, reverted the previous workaround and added support
back.

## Fix

`195973e9a..f24d67709`
* Temporarily drop support for `IA32_VMX_VMFUNC_MSR` due to regression
* Correct check for support for `IA32_VMX_VMFUNC_MSR`

