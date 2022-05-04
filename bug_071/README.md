# Support UEFI

## Scope
* x64 XMHF, (maybe x86 XMHF)
* New machines

## Behavior
On new machines, XMHF cannot boot, probably because new machines do not support
legacy mode. So XMHF needs to support UEFI mode.

## Debugging

### Trying to set up new HP machine

BIOS: F10
* Disable secure boot

Looks like Legacy boot not supported
* <https://h30434.www3.hp.com/t5/Business-Notebooks/Enable-legacy-mode-Elitebook-840-G7/td-p/7795277>:
> This is not possible anymore. G7 is HP's latest generation Elitebook
> configuration.
> Your PC uses UEFI and due to Intel decision, since 2020 UEFI is completely
> replacing BIOS. All legacy options are no longer present due to Intel decision (primary).

* <https://en.wikipedia.org/wiki/BIOS>:
> In 2017, Intel announced that it would remove legacy BIOS support by 2020.
> Since 2019, new Intel platform OEM PCs no longer support the legacy option.

* <https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface#UEFI_classes>:
> Class 0: Legacy BIOS
> Class 1: UEFI in CSM-only mode (i.e. no UEFI booting)
> Class 2: UEFI with CSM
> Class 3: UEFI without CSM
> Class 3+: UEFI with Secure Boot Enabled

### Running UEFI in QEMU

A good reference is <https://wiki.debian.org/SecureBoot/VirtualMachine>.

The key files are
* `/usr/share/edk2/ovmf/OVMF_CODE.fd` (read only)
* `/usr/share/edk2/ovmf/OVMF_VARS.fd` (read / write, should duplicate)

Arguments to add to `qemu-system-x86_64`:
* `-drive if=pflash,format=raw,readonly,file=OVMF_CODE.fd -drive if=pflash,format=raw,readonly,file=OVMF_VARS.fd` OR
* `-bios OVMF_CODE.fd`

Sample command:
```sh
qemu-system-x86_64 -m 512M \
	--drive media=cdrom,file=debian-11.0.0-amd64-netinst.iso,index=2 \
	-device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::1127-:22 \
	-smp 1 -cpu Haswell,vmx=yes --enable-kvm \
	-bios /usr/share/edk2/ovmf/OVMF_CODE.fd
```

Now we need to install a new Debian on GPT.

