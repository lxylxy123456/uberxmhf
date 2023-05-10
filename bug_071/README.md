# Support UEFI

## Scope
* x64 XMHF, (maybe x86 XMHF)
* New machines
* Git `ca07e6b54` (when creating this bug)
* Git `xmhf64 b8e4149e8` (when starting development)
* Git `xmhf64-dev b32eb891f`

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
longer connect to WiFi. The workaround is to use a USB network connection for
Debian.

### AMT version

Need to use amtterm version 1.7 or higher. e.g.
<https://rpmfind.net/linux/opensuse/tumbleweed/repo/oss/i586/amtterm-1.7-1.1.i586.rpm>

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

### UEFI interfaces

It is possible to implement XMHF as a UEFI driver. The logic will be
* UEFI -> XMHF -> UEFI -> OS (Windows / GRUB -> Linux)

The UEFI spec says
> A UEFI driver is not unloaded from memory if it returns a status code of
> `EFI_SUCCESS`.

If not using this method, can also use UEFI boot services' LoadImage and
StartImage functions.

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

### Debugging serial output

See `bug_073`. This may be a big project. We should continue working on
`bug_071` concurrently.

### Multiboot2

tboot-1.10.4's `tboot/common/boot.S` is a good reference for multiboot2 header.
Following this we can update `xmhf/src/xmhf-core/xmhf-bootloader/header.S`.

### UEFI development

OSDev has some good resources related to UEFI. Can try to follow them. We can
use GNU-EFI. Temporarily forget about XMHF.
* <https://wiki.osdev.org/UEFI>
* <https://wiki.osdev.org/GNU-EFI>
* <https://wiki.osdev.org/UEFI_App_Bare_Bones>

Install

```sh
sudo dnf install xorriso
```

Following tutorial
```sh
git clone https://git.code.sf.net/p/gnu-efi/code gnu-efi
cd gnu-efi
make
cd ..

# put tutorial provided file in main.c
gcc -Ignu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c main.c -o main.o
ld -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi -Tgnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o main.o -o main.so -lgnuefi -lefi
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi

dd if=/dev/zero of=fat.img bs=1k count=1440
mformat -i fat.img -f 1440 ::
mmd -i fat.img ::/EFI
mmd -i fat.img ::/EFI/BOOT
mcopy -i fat.img main.efi ::/EFI/BOOT

mkdir iso
cp fat.img iso
xorriso -as mkisofs -R -f -e fat.img -no-emul-boot -o cdimage.iso iso

cp /usr/share/edk2/ovmf/OVMF_{CODE,VARS}.fd .

qemu-system-x86_64 -cpu qemu64 -drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd -net none -enable-kvm -cdrom cdimage.iso

# In EFI Shell:
#  FS0:
#  cd EFI/boot
#  main.efi
```

Using this way, can boot rich OS (e.g. boot GRUB using `grubx64.efi`).

If place the `main.efi` to `/EFI/BOOT/BOOTX64.EFI` instead, then it will be
booted without entering the shell. However, after `main.efi` exits, the output
is gone, etc.

Other references
* <https://www.rodsbooks.com/efi-programming/efi_services.html>

#### Debugging using QEMU and GDB

Use LoadedImageProtocol to find out relocation offset. Follow
<https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB>. Suppose during
runtime "Image Base: 0x677a000" is printed. Then in GDB:
`symbol-file main.so -o 0x677a000`

Ref
* <https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB>

### EFI Shell on real hardware

Can use `/usr/share/edk2/ovmf/Shell.efi` to replace
`/boot/efi/EFI/fedora/grubx64.efi`, this will cause UEFI shell instead of GRUB
to be booted

Maybe can boot from a USB? Not tested:
<https://github.com/KilianKegel/HowTo-create-a-UEFI-Shell-Boot-Drive>

Using the `pci` command in UEFI shell, can see Intel AMT's serial port
(00:16.3 in Linux).

### `EFI_SERIAL_IO_PROTOCOL`

It is possible that Intel AMT can be accessed through UEFI using
`EFI_SERIAL_IO_PROTOCOL`. The idea is to first LocateHandle, then
HandleProtocol.

However, by trying on real machine, looks like this is not the case.
HP 2540p shows `LocateHandleBuffer` `EFI_SERIAL_IO_PROTOCOL` gives not found,
Dell 7050 gives one result (likely the physical serial port, not AMT).

Also, we are unable to call `HandleProtocol` correctly after
`LocateHandleBuffer`. Always see invalid parameter error not explainable by
the standard. Asking on <https://stackoverflow.com/questions/76096271/>.

### E820

We do not need to wrap UEFI's `GetMemoryMap` function. We simply need to make
XMHF bootloader a EFI runtime service. The XMHF bootloader can then call
`AllocatePool` with `Type=AllocateAddress`. This allows the XMHF BL to select
exact memory address to allocate.

### QEMU emulation failure

Starting to compile XMHF bl in EFI environment, developing in `xmhf64-dev`
branch.

In `xmhf64-dev 1b4bf7ffb`, if remove `efi.o` from Makefile (see following
patch), then build XMHF.
```diff
diff --git a/xmhf/src/xmhf-core/xmhf-bootloader/Makefile b/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
index f51d6e81d..e5d43e6a6 100644
--- a/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
+++ b/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
@@ -117,7 +117,7 @@ all: efi.efi fat.img cdimage.iso
 efi.o: efi.c
        $(CC) $(EFI_CFLAGS) -c $< -o $@
 
-efi.so: $(GNUEFI_BUILD)/x86_64/gnuefi/crt0-efi-x86_64.o efi.o $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS)
+efi.so: $(GNUEFI_BUILD)/x86_64/gnuefi/crt0-efi-x86_64.o $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS)
        $(LD) $(EFI_LDFLAGS) $^ $(EFI_LDLIBS) -o $@
        chmod -x efi.so
 
```

Then run QEMU using the following command
```sh
qemu-system-x86_64 -m 2G -drive if=pflash,format=raw,unit=0,file=/usr/share/OVMF/OVMF_CODE.fd,readonly=on -net none -cdrom ./xmhf/src/xmhf-core/xmhf-bootloader/fat.img -serial file:/dev/stdout -enable-kvm
```

See error in KVM
```
Press ESC in 5 seconds to skip startup.nsh or any other key to continue.
Shell> FS0:
FS0:\> cd EFI
FS0:\EFI\> cd BOOT
FS0:\EFI\BOOT\> load efi.efi
KVM internal error. Suberror: 1
extra data[0]: 0x0000000000000000
extra data[1]: 0x0000000000000030
extra data[2]: 0x0000000000000584
extra data[3]: 0x0000000000000000
extra data[4]: 0x0000000000000000
extra data[5]: 0x0000000000000000
emulation failure
RAX=0000000000000000 RBX=000000007fa24190 RCX=000000007fedaafd RDX=0000000000000004
RSI=000000007f9ec018 RDI=000000007e7823d8 RBP=000000007e780698 RSP=000000007fecd1f0
R8 =0000000000000004 R9 =0000000000000000 R10=0000000000000000 R11=0000000000000000
R12=000000007f9ec018 R13=000000007fa24190 R14=0000000000000001 R15=000000007e65acda
RIP=000000008ba1b049 RFL=00010287 [--S--PC] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0030 0000000000000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0038 0000000000000000 ffffffff 00a09b00 DPL=0 CS64 [-RA]
SS =0030 0000000000000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0030 0000000000000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0030 0000000000000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0030 0000000000000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 0000000000000000 0000ffff 00008200 DPL=0 LDT
TR =0000 0000000000000000 0000ffff 00008b00 DPL=0 TSS64-busy
GDT=     000000007f9dc000 00000047
IDT=     000000007f413018 00000fff
CR0=80010033 CR2=0000000000000000 CR3=000000007fc01000 CR4=00000668
DR0=0000000000000000 DR1=0000000000000000 DR2=0000000000000000 DR3=0000000000000000 
DR6=00000000ffff0ff0 DR7=0000000000000400
EFER=0000000000000d00
Code=?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? <??> ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??
```

Can further reduce the Makefile to:
```diff
diff --git a/xmhf/src/xmhf-core/xmhf-bootloader/Makefile b/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
index f51d6e81d..be85ace52 100644
--- a/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
+++ b/xmhf/src/xmhf-core/xmhf-bootloader/Makefile
@@ -118,7 +118,29 @@ efi.o: efi.c
 	$(CC) $(EFI_CFLAGS) -c $< -o $@
 
 efi.so: $(GNUEFI_BUILD)/x86_64/gnuefi/crt0-efi-x86_64.o efi.o $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS)
-	$(LD) $(EFI_LDFLAGS) $^ $(EFI_LDLIBS) -o $@
+	# $(LD) $(EFI_LDFLAGS) $^ $(EFI_LDLIBS) -o $@
+	ld -shared -Bsymbolic -L../../../../_build_gnuefi/x86_64/lib \
+	 -L../../../../_build_gnuefi/x86_64/gnuefi \
+	 -T../../../../xmhf/third-party/gnu-efi/gnuefi/elf_x86_64_efi.lds \
+	 ../../../../_build_gnuefi/x86_64/gnuefi/crt0-efi-x86_64.o \
+	 initsup.o \
+	 init.o \
+	 smp.o \
+	 cmdline.o \
+	 ../xmhf-runtime/xmhf-debug/lib.i386.a \
+	 ../xmhf-runtime/xmhf-tpm/tpm-interface.i386.o \
+	 ../xmhf-runtime/xmhf-tpm/arch/x86/tpm-x86.i386.o \
+	 ../xmhf-runtime/xmhf-tpm/arch/x86/svm/tpm-x86svm.i386.o \
+	 ../xmhf-runtime/xmhf-tpm/arch/x86/vmx/tpm-x86vmx.i386.o \
+	 ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pci.i386.o \
+	 ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pit.i386.o \
+	 ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-amd64-smplock.i386.o \
+	 ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.i386.o \
+	 ../../../../_build_libbaremetal32/_objects/libbaremetal.a \
+	 ../xmhf-runtime/xmhf-xmhfcbackend/xmhfc-putchar.i386.o \
+	 -lgnuefi \
+	 -lefi \
+	 -o efi.so
 	chmod -x efi.so
 
 efi.efi: efi.so
```

Versions
* Fedora, kernel `6.2.10-200.fc37.x86_64`
* `gcc (GCC) 12.2.1 20221121 (Red Hat 12.2.1-4)`
* `QEMU emulator version 7.0.0 (qemu-7.0.0-15.fc37)`

### Development progress

In `xmhf64-dev b32eb891f..195c7e6ca`, started porting bootloader to UEFI
environment. Now all bootloader code can compile under UEFI's PIC. The entry
point is starter code from GNU-EFI, instead of XMHF's `header.S`. Currently
most XMHF code do not run.

Rebase the above to `xmhf64 b8e4149e8..4f4c8b088` (the last commit contains
UEFI, other commits contain other things)

...

After some development, at `xmhf64 cb1f707e7`. We can boot Debian 11 x64
net install ISO with UP (SMP has a problem due to double INIT-SIPI-SIPI).

Possible problems
* Cannot reload TR. This will be a trouble if UEFI firmware uses TR. However
  it is possible to workaround this problem. See commit `cb1f707e7`.
* We need to assume UEFI firmware does not start other CPUs. Otherwise there is
  no way XMHF knows the state of other CPUs.

Maybe refer to newbluepill code to see how it initializes the VMCS guest state.

### Redesign INIT intercept handler

Currently, at `xmhf64 8cc6eea5d`. (KVM UEFI, XMHF, Debian 11 UP / Windows 10 UP)
can boot correctly.

Originally, XMHF only supports the guest to perform INIT-SIPI-SIPI once. The
next time XMHF see INIT, it assumes the guest is trying to shutdown.

However, this does not work on UEFI. When SMP Linux is booting, it first starts
all CPUs with INIT-SIPI-SIPI on xAPIC (using destination shorthand "All
Including Self"). Then it starts all CPUs again with INIT-SIPI-SIPI on x2APIC
(using destination shorthand "No Shorthand").

We need to modify how XMHF handles INIT signals after normal OS boots.
* When BSP receives INIT interrupt, assume guest OS is requesting shutdown.
* When AP receives INIT interrupt, let it wait in a while loop like in
  `xmhf_smpguest_arch_initialize`. If BSP detects an SIPI, it starts the AP.
  If BSP receives INIT interrupt, it resumes the AP to start shutdown process.

`8cc6eea5d..4c16e1435` contains relevant commit, then focused on other things.

...

Continue in `bug_121`.

### Real Serial Port

Try to debug UEFI on real hardware using traditional serial port (3f8).
However, see XMHF output gibberish.

Need to use putty (can install on Linux and Windows), set baud rate to 115200.
Instead of `cat /dev/ttyUSB0`. On Windows, need to install driver from USB
serial cable manufacturer's website. Sample command for Linux:
```sh
sudo plink -sercfg 115200 -serial /dev/ttyUSB0
```

To speed up development, can write UEFI shell script to automatically try XMHF:
```
FS0:

# Test whether should try XMHF
# sudo touch /boot/efi/XMHF_TRY
if exists \XMHF_TRY then
	rm \XMHF_TRY
	# Load XMHF
	load \EFI\BOOT\init-x86-amd64.efi
	# Sleep 1
	stall 1000000
endif

# Load Linux
\EFI\fedora\grubx64.efi_old
```

After some bug fixes, at `xmhf64 7be54b0e4`, can boot Fedora 37 SMP on Dell.

### Windows 10 on physical machine

To boot Windows, use `\EFI\Microsoft\Boot\bootmgfw.efi` in EFI shell. There is
another file called `bootmgr.efi`, but don't use it.

I tried to boot Windows on real machine, but receive error
`Unhandled intercept: 11`. SDM says it is related to GETSEC instruction. I
think it is likely that we need to hide this functionality.

To hide SMX (name of feature that provides GETSEC), we need to set
`CPUID.01H:ECX[6] = 0`. Implemented in `xmhf64 bfea3c42e`.

After that, Windows 10 UEFI can boot in Dell 7050. PAL demo also works well.

### Windows 11 on physical machine

Windows 11 says the PC does not meet minimum requirement, so cannot install
Windows 11 on Dell 7050. e.g.
<https://www.dell.com/community/Optiplex-Desktops/optiplex-7050-not-supporting-windows-11/td-p/8050523>

Need to bypass TPM and CPU check. Create a registry key with name
`AllowUpgradesWithUnsupportedTPMOrCPU` and value 1 in
`HKEY_LOCAL_MACHINE\SYSTEM\Setup\MoSetup`. Then restart and re-install using
ISO file.

Ref:
* <https://www.tomshardware.com/how-to/bypass-windows-11-tpm-requirement>
* <https://support.microsoft.com/en-us/windows/ways-to-install-windows-11-e0edbbfb-cfc5-4011-868b-2ce77ac7c70e>

After installing Windows 11, see XMHF run into `Unhandled intercept: 2` (triple
fault) during Windows 11 boot.

(Continue on `### Windows 11 on QEMU KVM` below)

### DRT in UEFI

Currently at `xmhf64 60f25cb2b`, find a strange problem. When DRT and UEFI are
enabled, the `rep stosb` instruction in `xmhf_sl_main()` is very slow. It takes
around 80 seconds (measured using RDTSC). When only UEFI is enabled, the
instruction takes around 0.03 seconds.

If I replace `rep stosb` with `memset()`, it is even slower. Around 580 seconds.
Is it related to caching?

Looks like the problem is related to caching. MTRR is not restored, so all
memory are not cached. Fixed in `xmhf64 232401115`.

Now the new problem is that after printing "Transferring control to runtime",
the machine resets (maybe triple fault?). See `TXT.ERRORCODE: 0xc0060000`.

The problem is that XMHF assumes the start of SL's .text section is `_mle_hdr`.
However, I previously modified Makefile and make it
`xmhf_sl_arch_x86_invoke_runtime_entrypoint`. Fixed in `xmhf64 74ba167a4`. Now
Fedora 35 can boot in XMHF UEFI DRT.

### Windows 11 on QEMU KVM

When installing, set memory to 8G and smp to 4.

We need to bypass Windows 11's TPM check to run on QEMU and KVM. Following
<https://www.tomshardware.com/how-to/bypass-windows-11-tpm-requirement>, use
Shift+F10 to open CMD, then open `regedit`.
* Go to `HKEY_LOCAL_MACHINE\SYSTEM\Setup`
* Create new key (like a folder) `LabConfig`, then go to it
* Create DWORD value `BypassTPMCheck`
* Create DWORD value `BypassSecureBoot`
* Create DWORD value `BypassRAMCheck`

In QEMU/KVM, can also reproduce the triple fault intercept in Windows 11.
Reproducible on UP. Error message looks like (Note Windows 11 has KASLR-like
feature):
```
CPU(0x00): Unhandled intercept: 2 (0x00000002)
        CPU(0x00): RFLAGS=0x0000000000010046
        SS:RSP =0x0000:0xfffff803344e0e50
        CS:RIP =0x0010:0xfffff80334ded292
        IDTR base:limit=0xfffff803344d4000:0x0fff
        GDTR base:limit=0xfffff803344d6fb0:0x0057
```

Interrupts are disabled because RFLAGS = 0x10046. Using GDB script `page.gdb`
to walk Windows 11's page table, can see that it is trying to execute
instruction `0x7c764292: xgetbv`

Intel SDM says possible cause of exceptions are:
* Invalid register specified in ECX
	* However, GDB shows `r->ecx == 0`, so this is not the problem
* `CPUID.01H:ECX.XSAVE[bit 26] = 0`
	* However, in KVM `CPUID.01H:ECX = 0xfffa3223`, so XGETBV is supported
* `CR4.OSXSAVE[bit 18] = 0`
	* Looks like this is the problem. There is `control_CR4_shadow = 0x668`,
	  `control_CR4_mask = 0x26e8`, and `guest_CR4 = 0x2000`.
* LOCK prefix is used
	* However, LOCK prefix is not used here

There are two possibilities
* Windows 11 intends to set CR4.OSXSAVE, but it is not set in XMHF.
* Windows 11 does not set CR4.OSXSAVE, and should receive `#UD`. However, for
  some reason triple fault occurs.

Using GDB, see below. Can see that all 512 entries in IDT are 0. So an `#UD`
exception will result in triple fault. Now the problem is why CR4.OSXSAVE is
not set.
```
(gdb) p/x vcpu->vmcs.guest_IDTR_limit 
$10 = 0xfff
(gdb) p/x vcpu->vmcs.guest_IDTR_base
$11 = 0xfffff806512a0000
(gdb) walk_pt 0xfffff806512a0000
$12 = "4-level paging"
$13 = "Page size = 4K"
$14 = 0x327a000
(gdb) x/10gx 0x327a000
0x327a000:	0x0000000000000000	0x0000000000000000
0x327a010:	0x0000000000000000	0x0000000000000000
0x327a020:	0x0000000000000000	0x0000000000000000
0x327a030:	0x0000000000000000	0x0000000000000000
0x327a040:	0x0000000000000000	0x0000000000000000
(gdb) 
```

#### Hiding XSAVE

I try to hide the XSAVE feature in `_vmx_handle_intercept_cpuid`. Then I see an
exception earlier in bootmgfw:
```
!!!! X64 Exception Type - 06(#UD - Invalid Opcode)  CPU Apic ID - 00000000 !!!!
RIP  - 000000007D9271F4, CS  - 0000000000000038, RFLAGS - 0000000000010346
RAX  - 00000000000306C4, RCX - 0000000000000000, RDX - 00000000078BFBFF
RBX  - 0000000000000800, RSP - 000000007FECD010, RBP - 000000007FECD030
RSI  - 000000000000000D, RDI - 0000000000000007
R8   - 00000000FFFFFD60, R9  - 000000007D76D0EA, R10 - 0000000000000007
R11  - 0000000000000013, R12 - 000000007D9D9A7C, R13 - 0000000000000000
R14  - 0000000000000001, R15 - 000000007D9D9C44
DS   - 0000000000000030, ES  - 0000000000000030, FS  - 0000000000000030
GS   - 0000000000000030, SS  - 0000000000000030
CR0  - 0000000080010033, CR2 - 0000000000000000, CR3 - 000000007FC01000
CR4  - 0000000000002668, CR8 - 0000000000000000
DR0  - 0000000000000000, DR1 - 0000000000000000, DR2 - 0000000000000000
DR3  - 0000000000000000, DR6 - 00000000FFFF0FF0, DR7 - 0000000000000400
GDTR - 000000007F9DC000 0000000000000047, LDTR - 0000000000000000
IDTR - 000000007F413018 0000000000000FFF,   TR - 0000000000000000
FXSAVE_STATE - 000000007FECCC70
!!!! Find image based on IP(0x7D9271F4) bootmgfw.pdb (ImageBase=000000007D746000, EntryPoint=000000007D7779C0) !!!!
```

It is possible to use GDB to break at RIP. The instruction is still XGETBV. We
can see that before executing XSETBV, CR4 is
`0x2668 [ VMXE OSXMMEXCPT OSFXSR MCE PAE DE ]`. However, if XMHF does not hide
XSAVE in CPUID, then CR4 is
`0x42668 [ OSXSAVE VMXE OSXMMEXCPT OSFXSR MCE PAE DE ]`. So XMHF cannot hide
XSAVE to workaround this problem.

#### Continue investigating

XMHF's `host_CR4 = 0x00042668`. This is set in
`xmhf_baseplatform_arch_cpuinitialize()`. UEFI firmware does not set this bit.
We probably need to debug Windows 11's boot process.

A workaround is shown in `xmhf64-dev 5a5f782ef`. When Windows triggers the
triple fault, we set CR4.OSXSAVE and retry the instruction. With this
workaround, Windows 11 can boot in QEMU/KVM UP. Can also boot in Dell 7050. PAL
demo also passes.

Plan for further investigation:
* Compare behavior of QEMU TCG and KVM XMHF
* Use QEMU TCG to record Windows 11's trace, and perform reverse debugging
  (otherwise it is hard to find a break point, because all addresses are
  randomized / relocatable)
* Use XMHF intercepts to find a break point, then start debugging using GDB /
  monitor trap. There are a lot of CPUID intercepts, but less other intercepts.

Continue in `bug_119`.

### DMAP

In `xmhf64 d2759ec0c`, fixed a bug in DMAP. Now when DMAP is enabled, Fedora
can run. However, Windows 11 runs into blue screen. Information using
BlueScreenViewer:
* Bug Check String: `KMODE_EXCEPTION_NOT_HANDLED`
* Bug Check Code: `0x0000001e`
* Parameter 1: `ffffffffc000001d`
* Parameter 2: `fffff8035dd01562` (likely randomized address)
* Parameter 3: `0000000000000000`
* Parameter 4: `fffff8035dd015a0` (likely randomized address)
* Caused By Driver: `intelppm.sys`
* Caused By Address: `intelppm.sys+15a0`
* File Description: Processor Device Driver
* File Version: `10.0.22621.1 (WinBuild.160101.0800)`
* Processor: x64
* Crash Address: ntoskrnl.exe+420fb0
* Major Version: 15
* Minor Version: 22621

Note: not tried Windows 11 DMAP on KVM yet.

Continue in `bug_120`.

### Summary

In this bug, developed a lot to enable UEFI support. However, there are still
3 problems remaining.
* Need to redesign INIT intercept handler to support INIT-SIPI-SIPI twice. This
  will allow running UEFI SMP in KVM. See
  `### Redesign INIT intercept handler`.
* Need to fix the bug that Windows 11 does not set CR4.OSXSAVE before XGETBV.
  See `#### Continue investigating`.
* Need to support Windows 11 when DMAP is enabled. See `### DMAP`.

Also, we need to
* Report "KVM internal error" bug to KVM

Also, an interesting question is
* How does GRUB installation make GRUB the default instead of Windows? This may
  be helpful to design how XMHF can be installed.

## Fix

`xmhf64 b8e4149e8..d2759ec0c`
* Support UEFI

`xmhf64-dev b32eb891f..945400857`
* Code for workaround CR4.OSXSAVE (reverted)

