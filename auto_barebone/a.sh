# From https://wiki.osdev.org/Bare_Bones

set -xe

i686-elf-as() {
	as --32 "$@"
}

i686-elf-gcc() {
	gcc -m32 "$@"
}

i686-elf-as boot.s -o boot.o
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

mkdir -p isodir/boot/grub
cp myos.bin isodir/boot/myos.bin
cp grub.cfg isodir/boot/grub/grub.cfg
grub2-mkrescue -o myos.iso isodir

echo qemu-system-i386 -cdrom myos.iso
