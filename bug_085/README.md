# Running XMHF in XMHF

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `eecb1ffce`
* Git branch `lhv` commit `6a8866c1e`

## Behavior
After `bug_084`, I think it should be possible to run XMHF in XMHF. So try it.

## Debugging

### Improving grub.py

`grub.py` needs to be updated for XMHF in XMHF to work. For example, currently
XMHF only boots correctly when it is the first disk. We also add some features,
like setting timeout (to debug GRUB).

In `xmhf64 3b8f57996..307d583ed`, rewrite most part of `grub.py` to implement
the above.

In previous work (undocumented?), `lhv-dev 803252ae3..a31c469d7` contains
`control.py` that automatically tests GRUB image with change to list of modules
copied. This was used to create a minimal GRUB image. We use a similar approach
to figure out the minimal GRUB image for booting Windows.

The result I get is
```
affs.mod
boot.mod
bufio.mod
chain.mod
command.lst
crypto.mod
disk.mod
drivemap.mod
extcmd.mod
gcry_tiger.mod
gettext.mod
gfxterm_background.mod
mmap.mod
msdospart.mod
nilfs2.mod
normal.mod
ntfs.mod
parttool.lst
parttool.mod
relocator.mod
terminal.mod
test.mod
tga.mod
verifiers.mod
video.mod
```

It looks difficult to make the GRUB image use the FAT file system. When I try
it, I get the "unknown filesystem" error from GRUB. So give up.

### Loading two XMHFs

Change `__TARGET_BASE_SL` to `0x20000000` for one XMHF to solve the address
collision problem. Looks like XMHF is taking much more memory than I expected.
So placing it below 256M (like LHV) is not realistic. We also need to increase
QEMU memory. Sample QEMU command is

```sh
./bios-qemu.sh -smp 1 -m 1G --gdb 2198 --qb32 -d build32 +1 -d build32lhv +1 -d debian11x86 +1
```

The first problem we see at `xmhf64-nest 88863b9ef` and `xmhf64 3f0c4c695` is:

```
Fatal: Halting! Condition 'cache01 == HPT_PMT_UC || cache01 == HPT_PMT_WB' failed, line 467, file arch/x86/vmx/nested-x86vmx-ept12.c
```

This problem happens in the outer XMHF. This is simply an unimplemented feature
in EPT.

TODO: make `__TARGET_BASE_SL` a configuration variable.
TODO

