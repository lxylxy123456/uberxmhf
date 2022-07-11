# Bochs

## Logistics (deprecated)

Install
```sh
sudo dnf install bochs
```

Run
```sh
bochs
```

Compile dependencies
```
sudo dnf install libXrandr-devel
```

Compile
```sh
# https://sourceforge.net/projects/bochs/files/bochs/2.7/bochs-2.7.tar.gz/download
wget https://iweb.dl.sourceforge.net/project/bochs/bochs/2.7/bochs-2.7.tar.gz
tar xf bochs-2.7.tar.gz
cd bochs-2.7/
# First part: from .conf.linux
# Second part: from osdev
./configure --enable-sb16 \
            --enable-ne2000 \
            --enable-all-optimizations \
            --enable-cpu-level=6 \
            --enable-x86-64 \
            --enable-vmx=2 \
            --enable-pci \
            --enable-clgd54xx \
            --enable-voodoo \
            --enable-usb \
            --enable-usb-ohci \
            --enable-usb-ehci \
            --enable-usb-xhci \
            --enable-busmouse \
            --enable-es1370 \
            --enable-e1000 \
            --enable-show-ips \
            \
            --enable-smp \
            --enable-debugger \
            --enable-debugger-gui \
            --enable-logging \
            --enable-fpu \
            --enable-cdrom \
            --enable-x86-debugger \
            --enable-iodebug \
            --disable-plugins \
            --disable-docbook \
            --with-x --with-x11 --with-term --with-sdl2 \
            \
            --prefix=USR_LOCAL
make -j `nproc`
make install
```

Configurations in bochsrc:
```
display_library: sdl2
#sound: ...
```

Boot CDROM: in bochsrc:
```
ata0-master: type=cdrom, path="debian-11.1.0-i386-netinst.iso", status=inserted
boot: cdrom, disk
```

Set up disk: use `bximage` (interactive), then in bochsrc:
```
ata1-master: type=disk, mode=flat, path="c.img", cylinders=0
```

Looks like dnf installed version is faster than self-compiled version.

## Compile guide after `bug_084`

```
sudo apt-get install libsdl2-dev

# https://sourceforge.net/projects/bochs/files/bochs/2.7/bochs-2.7.tar.gz/download
wget https://iweb.dl.sourceforge.net/project/bochs/bochs/2.7/bochs-2.7.tar.gz
tar xf bochs-2.7.tar.gz
cd bochs-2.7/
# set USR_LOCAL to installation path
./configure \
            --enable-all-optimizations \
            --enable-cpu-level=6 \
            --enable-x86-64 \
            --enable-vmx=2 \
            --enable-clgd54xx \
            --enable-busmouse \
            --enable-show-ips \
            \
            --enable-smp \
            --disable-docbook \
            --with-x --with-x11 --with-term --with-sdl2 \
            \
            --prefix=$USR_LOCAL
time make -j `nproc`
	# 12 sec on HP 840
make install
```

## Run guide after `bug_084`

Content of `bochsrc`:

```
cpu: model=corei7_sandy_bridge_2600k, count=2, ips=50000000, reset_on_triple_fault=1, ignore_bad_msrs=1, msrs="msrs.def"
memory: guest=512, host=256
romimage: file=$BXSHARE/BIOS-bochs-latest, options=fastboot
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
ata1: enabled=1, ioaddr1=0x170, ioaddr2=0x370, irq=15
ata0-master: type=disk, mode=flat, path="c.img"
ata1-master: type=disk, mode=flat, path="d.img"
boot: disk
log: log.txt
panic: action=ask
error: action=report
info: action=report
debug: action=ignore, pci=report # report BX_DEBUG from module 'pci'
debugger_log: -
com1: enabled=1, mode=file, dev=/dev/stdout
```

For more disks, use `ata0-slave` and `ata1-slave`.

## Resources
* <https://wiki.osdev.org/Bochs>
* <https://bochs.sourceforge.io/doc/docbook/user/index.html>
	* Boot CDROM error codes
	  <https://bochs.sourceforge.io/doc/docbook/user/bios-tips.html>
	* Pre-defined CPUs
	  <https://bochs.sourceforge.io/doc/docbook/user/cpu-models.html>
* <https://www.cnblogs.com/oasisyang/p/15358137.html>
* Slow
	* <https://sourceforge.net/p/bochs/discussion/39592/thread/45d675556a/>
	* <https://forum.osdev.org/viewtopic.php?f=1&t=31702>
	* <https://forum.osdev.org/viewtopic.php?f=1&t=31358>

