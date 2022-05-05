# Support UEFI

## Scope
* x64 XMHF, (maybe x86 XMHF)
* New machines
* Git `ca07e6b54`

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
> replacing BIOS. All legacy options are no longer present due to Intel
> decision (primary).

* <https://en.wikipedia.org/wiki/BIOS>:
> In 2017, Intel announced that it would remove legacy BIOS support by 2020.
> Since 2019, new Intel platform OEM PCs no longer support the legacy option.

* <https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface#UEFI_classes>:
> Class 0: Legacy BIOS
> Class 1: UEFI in CSM-only mode (i.e. no UEFI booting)
> Class 2: UEFI with CSM
> Class 3: UEFI without CSM
> Class 3+: UEFI with Secure Boot Enabled

### Installing Debian on new computer

Wifi drivers needed: `iwlwifi-QuZ-a0-hr-b0-{39..59}.ucode.xz`

This is actually upper and lower bound of the driver needed. Can only copy
`iwlwifi-QuZ-a0-hr-b0-59.ucode` to installer's `/lib/firmware`, then reload.

See `dmesg` for driver failure information.

### Configuring AMT on new computer

It looks like AMT needs to go through WiFi, which needs to be configured using
Windows (cannot use low-level interfaces to configure).

<https://www.intel.com/content/www/us/en/develop/documentation/amt-developer-guide/top/wifi/wifi-configuration-concepts.html>:
> The connection parameters for an Intel® Active Management Technology
> (Intel® AMT) wireless device closely resemble those required for the host OS
> to make a wireless connection.

Need to download some software
* Intel(R) PROSet/Wireless Software: looks like WiFi drivers, not what we need
* Intel(R) Setup and Configuration Software: probably what we need
	* Install Configurator will result in ACUConfig.exe, need to run in
	  PowerShell as admin.
	* Install ACUWizard will result in a graphical interface (what we want)

### AMT WiFi setting causes Debian iwlwifi `-110`

After applying WiFi settings using ACUWizard in Windows 10, Debian can no
longer

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

### Possible references

* Bareflank
	* <https://github.com/Bareflank/hypervisor>: a hypervisor framework?
	* <https://github.com/Bareflank/microv>: a hypervisor using the above
	* <https://github.com/ionescu007/SimpleVisor>: another hypervisor, probably
	  not guest-independent
* BitVisor
	* <https://github.com/matsu/bitvisor>

The disadvantage is that Bareflank does not seem to use Multiboot. However
it is likely the easiest to use Multiboot2 for XMHF.

### Multiboot2

<https://en.wikipedia.org/wiki/Multiboot_specification> says:
> As of July 2019, the latest version of Multiboot specification is 0.6.96,
> defined in 2009.[2] An incompatible second iteration with UEFI support,
> Multiboot2 specification, was later introduced. As of April 2019, the latest
> version of Multiboot2 is 2.0, defined in 2016.[4]

I believe the second sentence means that Multiboot2 is **incompatible** with
Multiboot, but compatible with UEFI. So the plan is now to replace Multiboot
with Multiboot2. We still try to use the normal boot logic:
* Old logic: BIOS -> GRUB --(multiboot)-> XMHF -> GRUB -> OS
* New logic: UEFI -> GRUB --(multiboot2)-> XMHF -> GRUB -> OS

### Booting

In QEMU, booting is similar. Just change the disk location from `(hd0,msdos1)`
to `(hd0,gpt2)`. Both should still point to `/` in Linux. `(hd0,gpt2)` points
to `/boot/efi`.

The first problem is that bootloader stucks at a spinlock.

```
symbol-file xmhf/src/xmhf-core/xmhf-bootloader/init_syms.exe
```

Debugging is difficult, because we are working on bootloader (32-bit), and we
need to use `qemu-system-x86_64` (required by UEFI).

Actually the problem starts when we see
`System is not ACPI Compliant, falling through...` in serial output. We need
multiboot2 to receive the EFI system table / ACPI table. XMHF should be able
to use `EFI_ALLOCATE_PAGES` to reserver the memory for sl and rt
(the memory type is `EfiLoaderData`, which a legitimate guest OS should not
touch). This is actually cleaner than modifying BIOS e820.

One thing to notice is that after entering SL, XMHF cannot call EFI functions
anymore. These functions are at unprotected memory and may be modified by guest
OS.

TODO: use multiboot2

