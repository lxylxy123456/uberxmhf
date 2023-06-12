# Small Hyper Visor (SHV)

## Scope
* `lhv 05ed913cb6`

## Behavior
LHV is still architecturally dependent on XMHF. We want to make something
smaller, so it is easier to share with the open source community. Especially,
we should skip SL+RT, and make bootloader be the one that spawns the VMX.

## Debugging

Current build and run commands:
```sh
./build.sh i386 --lhv-opt 0xdfd && python3 ./tools/ci/grub.py --subarch i386 --xmhf-bin "." --work-dir "." --boot-dir "./tools/ci/boot/"
../tmp/bios-qemu.sh -d grub/c.img +1
```

`bios-qemu.sh` is opened source in `notes` branch, with name `kvm.sh`. Now the
command becomes:
```
../notes/kvm.sh -d grub/c.img +1
```

New branch for SHV: `shv`

### Autoconf

We follow these to remove XMHF dependency, hoping the code base becomes smaller
* <https://wiki.osdev.org/Bare_Bones>
* <http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/>

In directory `auto_barebone`, combine OSDev bare bone and autoconf. Then port
SHV to use this framework.

