set -xe

# Debian: apt install xorriso mtools grub-pc-bin

autoreconf --install && ./configure --host=i686-linux-gnu && make

set +x

echo qemu-system-i386 -cdrom grub.iso
echo qemu-system-i386 -kernel shv.bin

