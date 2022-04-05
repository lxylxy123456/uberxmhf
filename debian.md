## Debian

### Download ISO
<https://cdimage.debian.org/debian-cd/current/i386/iso-cd/debian-11.1.0-i386-netinst.iso>

### QEMU
```sh
qemu-img create -f qcow2 a.qcow2 32G
qemu-system-i386 -m 512M \
	--drive media=cdrom,file=debian-11.1.0-i386-netinst.iso,index=1 \
	--drive media=disk,file=a.qcow2,index=2 \
	--enable-kvm \
	-device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::1126-:22 \
	-smp 4 -serial stdio -cpu Haswell,pae=no,vmx=yes
```

Disable PAE: `--cpu=qemu32,pae=no`

### Install packages
```sh
apt-get install -y sudo libc6-i386    (others same as Ubuntu)
```

### Install non-PAE kernel
```sh
apt-get install -y linux-image-686
```

### Disable NTP
Looks like NTP will cause reboot to be slow. We can disable it:

```sh
sudo timedatectl set-ntp 0
```

### Compile XMHF
Warnings generated because high gcc version. Manually fix the code

### Serial port
```sh
$ sudo dmesg | grep ttyS
[    1.472013] 00:04: ttyS0 at I/O 0x3f8 (irq = 4, base_baud = 115200) is a 16550A
```

Change grub line to the following, then can see XMHF output from serial port
```
	multiboot ${kernel} serial=115200,8n1,0x3f8
```

### Changing environment
* Normal TrustVisor
	* Receive error `FATAL ERROR: TXT not suppported`
* Disable DRT and use helloworld (a simpler hypapp)
	* QEMU CPU does not support virtualization
		* Receive error `CPU(0x01) does not support VT. Halting!`
	* QEMU CPU does supports virtualization
		* Receive error `CPU(0x00): VMWRITE failed. HALT!`

### VMX on QEMU
Ref:
<https://stackoverflow.com/questions/39154850/how-do-i-emulate-the-vmx-feature-with-qemu>

Looks like `vmx` support in QEMU is implemented by nested vmx (the Ben-Yehuda
paper), and QEMU does not support emulating vmx. Also since XMHF does not work
out of the box, using QEMU is deprecated. But it can assist a lot of debugging.

### Debugging Debian on QEMU

Install symbol file using `apt-get install linux-image-$(uname -r)-dbg`
(cached in `/var/cache/apt/archives/`, installed files in `/usr/lib/debug`)

`sudo apt-get install dpkg-dev`

Install source file using `apt-get source linux-image-$(uname -r)-dbg`
(installed files in `./linux-<version-number>`)

`cd` into source file directory, then in GDB
`symbol-file .../usr/lib/debug/boot/vmlinux-<version>`

Should be able to also use `directory` command in gdb
* Ref: <https://sourceware.org/gdb/onlinedocs/gdb/Source-Path.html>

For example, normal files are `arch/x86/mm/extable.c`. But sometimes I see
`/build/linux-Zn7N0z/linux-5.10.84/arch/x86/kernel/head_32.S`. So creating
a symbolic link at `./build/linux-Zn7N0z` with content `../..` solves it.

### Debian 11 on HP

Get unhandled exception when OS is booting in XMHF
```
[00]: unhandled exception d, halting!
[00]: state dump follows...
[00] CS:EIP 0x0008:0x10206c01 with EFLAGS=0x00010016
[00]: VCPU at 0x10244aa0
[00] EAX=0x00000491 EBX=0xcab88360 ECX=0x00000491 EDX=0xffffc0b0
[00] ESI=0x10244aa0 EDI=0x0000007f EBP=0x1994bf1c ESP=0x1994bf0c
[00] CS=0x0008, DS=0x0010, ES=0x0010, SS=0x0010
[00] FS=0x0010, GS=0x0010
[00] TR=0x0018
[00]-----stack dump-----
...
```

### VirtualBox

VirtualBox has a debugger, not sure whether useful.

```
vboxmanage modifyvm "$NAME" --nested-hw-virt on
vboxmanage startvm "$NAME" -E VBOX_GUI_DBG_AUTO_SHOW=true -E VBOX_GUI_DBG_ENABLED=true
```

* Ref: <https://www.virtualbox.org/manual/ch12.html#ts_debugger>
* Ref: <https://stackoverflow.com/a/53286104>

Need to change CPUID, but not successful

```
vboxmanage setextradata "$NAME" VBoxInternal/CPUM/HostCPUID/00000000/ebx 0x756E6547
vboxmanage setextradata "$NAME" VBoxInternal/CPUM/HostCPUID/00000000/edx 0x49656E69
vboxmanage setextradata "$NAME" VBoxInternal/CPUM/HostCPUID/00000000/ecx 0x6C65746E
```
* Ref: <https://superuser.com/a/774596>

