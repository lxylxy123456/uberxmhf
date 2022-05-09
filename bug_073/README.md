# Support PCI Serial in UEFI

## Scope
* x64 XMHF
* UEFI only
* HP
* Git `63240f0ef`

## Behavior
In UEFI, existing code cannot write to the serial over lan.

## Debugging

Note: this is a subproject of `bug_071`.

After configuring the new laptop, can
* Use a separate USB wifi card to connect to the Internet in Debian
* Write to serial port in Debian, see output from debugging machine

However
* Create multiboot GRUB entry, but cannot see serial output from XMHF

After adding `console=tty0 console=ttyS0,115200`, Debian's boot messages appear
in serial port.
* Ref: <https://wiki.archlinux.org/title/working_with_the_serial_console>

Next we try to write to serial in GRUB, which is lower level than Debian.
However,

Looks like we can reference tboot. The serial port code in XMHF comes from
tboot, and the later version of tboot uses multiboot2. Install tboot on Debian:
`apt-get install tboot`.

However, tboot also does not have serial outputs. We may need to review
whether tboot serial setup code is correct.

Next steps:
* Test tboot on QEMU
* Compare tboot serial code with <https://wiki.osdev.org/Serial_Ports>
* Explore Linux code (e.g. are there information in `/sys`?)
* Write simple EFI app: <https://www.rodsbooks.com/efi-programming/hello.html>
* Try to access serial information from UEFI: `EFI_SERIAL_IO_PROTOCOL`
* Try to print to screen: `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL`
* See <https://www.kernel.org/doc/html/latest/ia64/serial.html>
* Read Linux source: `drivers/tty/serial/8250/8250_{early,core}.c`

In Linux, `/sys/devices/*/*/tty/ttyS0` contains information about ttyS0.
However, cannot see the difference between QEMU and HP 840 that breaks the
serial port.

Interesting comments from
<https://github.com/Bareflank/hypervisor#important-tips>:
> Using PCI serial addon cards will not work with UEFI. These cards need to be
> initialized by the OS, logic that Bareflank does not currently contain. If
> you are using Bareflank directly from Windows/Linux, these cards will work
> fine, but from UEFI, you need a serial port provided on the motherboard.

Maybe this is related. On QEMU, the serial port is at
`/sys/devices/pnp0/00:04/tty/ttyS0`. But in HP it is at
`/sys/devices/pci0000:00/0000:00:16.3/tty/ttyS0`. So it is probably a PCI
device.

### Linux behavior

As mentioned before, `console=ttyS0,115200` works well.

`console=ttyS0,9600` also works.

`console=uart8250,io,0x3060,115200n8` does not work. The start of messages seen
are:

```
[    2.311240] 0000:00:16.3: ttyS0 at I/O 0x3060 (irq = 19, base_baud = 115200) is a 16550A
[    2.313644] printk: console [ttyS0] enabled
[    2.313644] printk: console [ttyS0] enabled
[    2.328964] printk: bootconsole [uart8250] disabled
[    2.328964] printk: bootconsole [uart8250] disabled
[    2.329833] hpet_acpi_add: no address or irqs in _CRS
[    2.330252] Linux agpgart interface v0.103
[    2.330599] AMD-Vi: AMD IOMMUv2 functionality not available on this system - This is not a bug.
[    2.331817] i8042: PNP: PS/2 Controller [PNP0303:PS2K] at 0x60,0x64 irq 1
...
```

Can see that messages before this are lost.

`console=uart8250,io,0x3f8,115200n8` works in QEMU.

```
[    0.000000] Linux version 5.10.0-14-amd64 (debian-kernel@lists.debian.org) (gcc-10 (Debian 10.2.1-6) 10.2.1 20210110, GNU ld (GNU Binutils for Debian) 2.35.2) #1 SMP Debian 5.10.113-1 (2022-04-29)
[    0.000000] Command line: BOOT_IMAGE=/boot/vmlinuz-5.10.0-14-amd64 root=UUID=135524c4-8cbe-4c3f-85c2-0301e1f18e32 ro console=uart8250,io,0x3f8,115200n8
...
[    0.000000] earlycon: uart8250 at I/O port 0x3f8 (options '115200n8')
[    0.000000] printk: bootconsole [uart8250] enabled
...
[    0.244290] Console: colour dummy device 80x25
[    0.245397] printk: console [ttyS0] enabled
[    0.245397] printk: console [ttyS0] enabled
[    0.247386] printk: bootconsole [uart8250] disabled
[    0.247386] printk: bootconsole [uart8250] disabled
```

Asked question: <https://stackoverflow.com/questions/72166653/>

### Quick Boot

The "Quick Boot by Peter Dice" (<https://doi.org/10.1515/9781501506819>) book
mentioned in 15410 looks useful.

Looks like BIOS has does a lot of work for PCI. Maybe when booting without UEFI
the BIOS already sets up the serial ports. However in UEFI it no longer works.

### QEMU

Also it looks like QEMU supports a device called "pci-serial". Maybe we can use
this to debug the serial port on QEMU.

<https://serverfault.com/questions/872238/> contains an example of how to
configure this device.

```
qemu-system-x86_64 ... \
	-chardev file,id=mychardev,path=/tmp/ps -device pci-serial,chardev=mychardev
```

This works well in Linux. Can also verify that writing to `/dev/ttyS1` works.

```
$ sudo dmesg | grep ttyS
[    0.963325] 00:04: ttyS0 at I/O 0x3f8 (irq = 4, base_baud = 115200) is a 16550A
[    0.983771] 0000:00:04.0: ttyS1 at I/O 0xc050 (irq = 11, base_baud = 115200) is a 16550A
$ ls -d /sys/devices/*/*/tty/ttyS?
/sys/devices/pci0000:00/0000:00:04.0/tty/ttyS1
/sys/devices/platform/serial8250/tty/ttyS2
/sys/devices/platform/serial8250/tty/ttyS3
/sys/devices/pnp0/00:04/tty/ttyS0
$ 
```

However, XMHF's serial port output also works. I guess that maybe the QEMU's
UEFI firmware is performing the initialization. But HP's UEFI firmware is not.

TODO: check stackoverflow
TODO: maybe ask BareFlake people
TODO: see whether UEFI provides serial initialization (back to `bug_071`)
TODO: see whether UEFI provides PCI support
TODO: read Linux code, see how PCI is initialized
TODO: read OVMF code, see whether it touches PCI

