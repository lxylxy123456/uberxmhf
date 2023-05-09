# Windows 11 CR4.OSXSAVE

## Scope
* Windows 11 UEFI (also other OS, see below)
* Both UP and SMP
* KVM and Dell 7050
* `xmhf64 d2759ec0c`
* `xmhf64-dev 945400857`
* `lhv db7157160b`
* `lhv-dev df6578f6ce`

## Behavior

From `bug_071`

When booting Windows 11, CPU 0 see triple fault, because guest executes XGETBV
without setting CR4.OSXSAVE.

## Debugging

For previous debug information see `bug_071`, section
`#### Continue investigating`

### XSETBV

CR4.OSXSAVE also manages XSETBV. In `_vmx_handle_intercept_xsetbv`, we see the
comment showing that XSETBV's intercept handler is incomplete. In
`xmhf64-dev 53ca2f1b1`, we add check for XSETBV (CPL=0, CR4.OSXSAVE=1). We then
see CI failing. For example,
`./bios-qemu.sh -d build64 +1 -d debian11x64 +1 --gdb 2198 -m 2G -smp 1` will
fail.

This is a good news, because we can debug using Linux, which is open source.

### XSETBV not triggering VMEXIT

Question: does XSETBV really cause VMEXIT when CPL > 0?

I suspect the Intel SDM has a typo.
"24.1.2 Instructions That Cause VM Exits Unconditionally" says:
> The following instructions cause VM exits when they are executed in VMX
> non-root operation: ..., and XSETBV

However, looks like when CPL > 0 and CR4.OSXSAVE = 1, XSETBV does not trigger
VMEXIT. Instead, the user mode process receives #GP(0). This can be
demonstrated in `lhv-dev eb60c88911`. Commands:
```sh
./build.sh amd64 --lhv-opt 0xa00 && gr
../bios-qemu.sh -d build64lhv +1 -smp 1
```

Actually this is not a problem. Intel v3
"24.1.1 Relative Priority of Faults and VM Exits" says:
> Certain exceptions have priority over VM exits. These include invalid-opcode
> exceptions, faults based on
> privilege level,1 and general-protection exceptions that are based on
> checking I/O permission bits in the task-state segment (TSS). For example,
> execution of RDMSR with CPL = 3 generates a general-protection exception
> and not a VM exit.2

### Debian CR4.OSXSAVE

Actually this is a bug in testing code. To compute the CR4 observed by the
guest, should use `(mask & shadow) | (~mask & guest_CR4)`. However, the
preivous code uses `shadow`. Fixed in `xmhf64-dev 7d13c32a8`. Ported in
`xmhf64 d3c51e3ca`. Now CI passes (Debian 11, Windows 7 - 10 pass).

### CPUID `OS*` fields

I guess that the logic of Windows 11 is:
* Check `CPUID.01H:ECX.OSXSAVE[bit 27]`
* If this bit is not set, set it using `CR4.OSXSAVE[bit 18]`

However, when exporting CPUID, XMHF returns the OSXSAVE bit of host mode, not
guest mode. Thus, Windows 11 see OSXSAVE=1, though CR4.OSXSAVE is 0. Thus,
Windows 11 does not set `OSXSAVE`, which causes the bug.

We need to find bits in CPUID that can be changed by the guest software and can
be different from the host. Using two ways:
1. Search for "OS" in Intel v2 CPUID
2. Search for "CPUID" in Intel v3 "2.5 CONTROL REGISTERS"

We need to modify the logic for the following CPUID bits:
* `CPUID.01H:ECX.OSXSAVE[bit 27]` (`CR4.OSXSAVE[bit 18]`)
* `CPUID.(EAX=07H,ECX=0H):ECX.OSPKE[bit 4]` (`CR4.PKE[bit 22]`)
* `CPUID.19H:EBX.AESKLE[bit 0]` (`CR4.KL[bit 19]`)

Fixed in `xmhf64 fed5b4097..64db0f61f`. Now Windows 11 UEFI can boot in KVM UP
and Dell 7050 SMP.

## Fix

`xmhf64 d2759ec0c`
* Check state for XSETBV
* Compute CPUID values that depend on CR4

