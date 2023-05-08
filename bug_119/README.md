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

TODO

### Debian CR4.OSXSAVE

TODO

TODO: investigate `### XSETBV not triggering VMEXIT`, report to Intel

