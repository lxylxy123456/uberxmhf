# QEMU / KVM / GDB cannot read nested guest that has non-identity mapped EPT

## Scope
* Likely not related to XMHF
* Linux `6.0.15-300.fc37.x86_64`
* `xmhf64 5f7babfd8`
* `lhv ee68e5ac1`
* `lhv-dev 27b1834e0`

## Behavior

When debugging GreenBox, cannot read green memory space. Looks like this is a
problem with the KVM / QEMU.

## Debugging

The behavior from debugging GreenBox side is:

```
Use 64-bit paging
*(guest physical address)0x0000 = 0x1067
*(guest physical address)0x1000 = 0x2067
*(guest physical address)0x2000 = 0x3067
*(guest physical address)0x3000 = 0x4025
*(guest physical address)0x4000 = 0xf1f1f1f1f1f1f1f1
vcpu->vmcs.guest_CR3 = 0
vcpu->vmcs.guest_RIP = 0x4
Set a hardware breakpoint at 0x4 using GDB
If EPT makes identity mapping, then can access gva 0x0 from QEMU
If EPT makes mapping x -> x + 0xdddd000, can't access gva 0x0 from QEMU
```

We are able to reproduce this problem in `lhv-dev db6328011`. Use
`./build.sh amd64 --lhv-opt 0x4 && gr` to build. Run with
`../tmp/bios-qemu.sh -d build64lhv +1 -smp 1 --gdb 1234`. After LHV halts go to
GDB. See RIP = 2.

By default `u64 offset = 0x1000;`. In GDB `x $rip` gives
"Cannot access memory at address 0x2". If change offset to 0, GDB gives
0xf4f4f4f4 as expected.

In QEMU monitor, `x 2` has similar behavior.

TODO: see whether the problem happens because we change EPTP. i.e. perform this experiment at the first VMLAUNCH, not at a consequent VMRESUME
TODO: report bug to QEMU / KVM

