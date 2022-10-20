# Bug in bootloader copying hypervisor image

## Scope
* Reproducible in amd64, QEMU
* Applies to all configurations
* `xmhf64 0a9c369d4`
* `xmhf64-nest b4fd705b2`

## Behavior

Circle CI caught an error in
<https://app.circleci.com/pipelines/github/lxylxy123456/uberxmhf/1378/workflows/2143fc83-32ec-4702-9c1e-5a6cf4c44fae/jobs/2526>.
The runtime halts early by printing:
```
xmhf_baseplatform_arch_initialize: ACPI RSDP not found, Halting!
```

## Debugging

This problem is reproducible on ThinkPad using KVM.

Build command:
* bad: `./build.sh amd64 circleci --no-x2apic && gr`
* bad: `./build.sh amd64 && gr`
* good: `./build.sh amd64 fast && gr`
* bad: `./build.sh amd64 --no-bl-hash && gr`

Run command:
* bad: `./bios-qemu.sh -m 1G -smp 2 -d build64 +1 -d debian11x64 -t`
* bad: `./bios-qemu.sh -m 1G -smp 1 -d build64 +1 -d debian11x64 -t`

Version:
* `b4fd705b2`: bad
* `47e22a346`: good

Through delta debugging, in `47e22a346`, changing `VMX_NESTED_MAX_ACTIVE_VMCS`
from 4 to 8 causes the problem. By observing the `runtime.exe` generated, can
see that the new `.bss` ends at 4K page boundary.

Then debug using GDB, comparing bad and good version.
`xmhf_baseplatform_arch_x86_acpi_getRSDP()` should return 0xf59a0, but in the
bad version RSDP is not found because reading the memory failed. The failure is
caused by the fact that `xmhf_baseplatform_arch_flat_va_offset != 0` in
runtime.

Then we can set a (write) watch point on
`xmhf_baseplatform_arch_flat_va_offset`. Looks like `memcpy()` in bootloader
copies non-zero value to this address, even though this address should be in
`.bss`.
```
1105      //relocate the hypervisor binary to the above calculated address
1106      memcpy((void*)hypervisor_image_baseaddress, (void*)mod_array[0].mod_start, sl_rt_size);
```

Then by print debugging, looks like `hypervisor_image_baseaddress` and
`mod_array[0].mod_start` overlap. By changing `memcpy` to `memmove`, the
problem is fixed. See `xmhf64 a02353bef`.

Found 2 follow up problems:
1. Should test other elements in `mod_array`. Do the overlap with sl+rt?
2. After `bug_095`, may need to sort sections in some way to prevent
   unnecessary fragmentation

Problem 1 is fixed in `xmhf64 fd9e3e3bb`. At `xmhf64-nest 2c850ad3e`, Circle CI
passes.

### Linker sort order

For problem 2, we need to study related options for linker script. See official
documentation in
<https://sourceware.org/binutils/docs-2.39/ld/Input-Section-Wildcards.html>.

Looks like we can use `SORT_BY_ALIGNMENT`. The syntax is
`*(SORT_BY_ALIGNMENT(.bss.*))`. Note that even though it is not
`SORT_BY_ALIGNMENT(*(.bss.*))`, all page-aligned sections across all input
files will be sorted. Not only sorting each file locally.

Fixed in `xmhf64 c36d37b38`.

## Fix

`xmhf64 0a9c369d4..c36d37b38`
* Fix bug when copying hypervisor image
* Add check to overlap of multiboot modules
* Sort bss sections by alignment to reduce fragmentation

