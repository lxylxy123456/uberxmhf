set -xe

# Debian: apt install xorriso mtools grub-pc-bin

autoreconf --install && ./configure --host=i686-linux-gnu && make -j `nproc`
autoreconf --install && ./configure --host=x86_64-linux-gnu && make -j `nproc`

set +x

echo qemu-system-i386 -cdrom grub.iso
echo qemu-system-x86_64 -kernel shv.bin -serial stdio -display none

