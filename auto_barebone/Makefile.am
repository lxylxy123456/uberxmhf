bin_PROGRAMS = myos.bin
myos_bin_SOURCES = boot.s kernel.c

myos_bin_CCASFLAGS = -m32
myos_bin_CFLAGS = -m32
myos_bin_LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib
myos_bin_LDADD = -lgcc
#myos_bin_LDLIBS = -lgcc

