# Template from http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/

bin_PROGRAMS = shv.bin
shv_bin_SOURCES = boot.s kernel.c \
	debug.c debug-uart.c debug-vga.c \
	libc_string.c libc_stdio.c \
	lhv-asm.S lhv.c lhv-console.c lhv-ept.c lhv-guest-asm.S lhv-guest.c \
	lhv-keyboard.c lhv-mouse.c lhv-pic.c lhv-timer.c lhv-user-asm.S lhv-user.c \
	lhv-vmcs.c lhv-vmx.c 

shv_bin_CCASFLAGS = -m32 -Iinclude/
shv_bin_CFLAGS = -m32 -Iinclude/
shv_bin_LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib
shv_bin_LDADD = -lgcc

all: grub.iso

grub.iso: shv.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp shv.bin isodir/boot/shv.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub2-mkrescue -o grub.iso isodir

clean-local: clean-local-grub

clean-local-grub:
	rm -f grub.iso
	rm -rf isodir/

