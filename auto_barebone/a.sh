set -xe

autoreconf --install && ./configure && make

set +x

echo qemu-system-i386 -cdrom grub.iso
echo qemu-system-i386 -kernel shv.bin

