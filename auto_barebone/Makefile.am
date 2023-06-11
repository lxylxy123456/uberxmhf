# Template from http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/

bin_PROGRAMS = myos.bin
myos_bin_SOURCES = boot.s kernel.c

myos_bin_CCASFLAGS = -m32
myos_bin_CFLAGS = -m32
myos_bin_LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib
myos_bin_LDADD = -lgcc

all: grub

grub: myos.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub2-mkrescue -o myos.iso isodir

clean-local: clean-local-grub

clean-local-grub:
	rm -f myos.iso
	rm -rf isodir/

