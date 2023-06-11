set -xe

autoreconf --install && ./configure && make

set +x

echo qemu-system-i386 -cdrom myos.iso
echo qemu-system-i386 -kernel myos.bin

