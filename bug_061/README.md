# Allow secure loader + runtime to be placed at configurable places

## Scope
* All XMHF
* Git `85141ca6b`

## Behavior
Secureloader's start address is defined in `__TARGET_BASE_SL`. Runtime's is
defined in `__TARGET_BASE`. These values should be configurable. However,
currently after changing them from `0x10000000` to `0x20000000`, there is some
kind of triple fault after seeing `Transferring control to runtime` in serial.

## Debugging
Git `ce1275094` changes the start address from 256M to 512M.

Using `-no-shutdown -no-reboot` stops QEMU before triple fault. Can see that
the problem happens `xmhf_sl_arch_x86_invoke_runtime_entrypoint()` at
`movw %ax, %ds`. We can imagine that there is some problem with runtime GDT.

GDT is passed in `XtVmmGdt`. We need to see its value in
`xmhf_sl_arch_xfer_control_to_runtime()`.

Note that GDB scripts need to be changed, because SL address has changed.

```
# Stop at beginning of SL (before translating address)
set confirm off
set pagination off
b *0x20003080
c
d
symbol-file -o 0x20000000 xmhf/src/xmhf-core/xmhf-secureloader/sl_syms.exe

# This is the jump to sl_startnew
# There is no global symbol, so need to manually find its address
b *0x20000000 + 0x3174
c
d
si
# Now at 0x317b
symbol-file
symbol-file xmhf/src/xmhf-core/xmhf-secureloader/sl_syms.exe

hb *xmhf_sl_arch_x86_invoke_runtime_entrypoint
hb *xmhf_sl_arch_x86_invoke_runtime_entrypoint + 0x20000000
c
d
```

Note that GDB does not know about non-zero segment base. So printing C
variables become unreliable. To see RPB, for example, use
`p/x *(RPB*)0x20200000`

```
(gdb) p/x *(RPB*)0x20200000
$11 = {magic = 0xf00ddead, XtVmmEntryPoint = 0x10210f84, XtVmmPdptBase = 0x1d03b000, XtVmmPdtsBase = 0x1d03c000, XtGuestOSBootModuleBase = 0x100000, XtGuestOSBootModuleSize = 0x200, runtime_appmodule_base = 0x0, runtime_appmodule_size = 0x0, XtGuestOSBootDrive = 0x80, XtVmmStackBase = 0x1d060000, XtVmmStackSize = 0x2000, XtVmmGdt = 0x1024a720, XtVmmIdt = 0x10246130, XtVmmIdtFunctionPointers = 0x10246540, XtVmmIdtEntries = 0x20, XtVmmRuntimePhysBase = 0x20200000, XtVmmRuntimeVirtBase = 0x20200000, XtVmmRuntimeSize = 0xce64000, XtVmmE820Buffer = 0x102465c0, XtVmmE820NumEntries = 0x9, XtVmmMPCpuinfoBuffer = 0x10246ac0, XtVmmMPCpuinfoNumEntries = 0x1, XtVmmTSSBase = 0x102490c0, RtmUartConfig = {baud = 0x1c200, data_bits = 0x8, parity = 0x0, stop_bits = 0x1, fifo = 0x0, clock_hz = 0x1c2000, port = 0x3f8}, cmdline = {0x73, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x3d, 0x31, 0x31, 0x35, 0x32, 0x30, 0x30, 0x2c, 0x38, 0x6e, 0x31, 0x2c, 0x30, 0x78, 0x33, 0x66, 0x38, 0x20, 0x62, 0x6f, 0x6f, 0x74, 0x5f, 0x64, 0x72, 0x69, 0x76, 0x65, 0x3d, 0x30, 0x78, 0x38, 0x30, 0x0 <repeats 985 times>}, isEarlyInit = 0x1}
(gdb) 
```

We can see that most pointers are wrong. Then I realized that
`runtime-x86-i386.lds.S` and `runtime-x86-amd64.lds.S` also need to be updated.
Maybe we want to run the preprocessor on lds files.

Fixed in `85141ca6b..69ff2c15c`. After the fix, only `__TARGET_BASE_SL` need to
be updated. Also, merged LD scripts for different subarchs for runtime and sl

## Fix
`85141ca6b..69ff2c15c`
* Remove magic numbers depending on `__TARGET_BASE_SL`
* Merge LD scripts across different subarchs

