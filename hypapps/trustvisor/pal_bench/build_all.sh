#!/bin/bash
# Run in Debian container

set -xe

make clean
rm -f pal_bench{32,64}{,L2}

MKFLAGS=()
MKFLAGS+=("CC=i686-linux-gnu-gcc" "LD=i686-linux-gnu-ld")

make "${MKFLAGS[@]}" TV_H=32
mv main pal_bench32
make clean

make "${MKFLAGS[@]}" TV_H=64
mv main pal_bench64
make clean

make "${MKFLAGS[@]}" TV_H=32 VMCALL_OFFSET=0x4c415000U
mv main pal_bench32L2
make clean

make "${MKFLAGS[@]}" TV_H=64 VMCALL_OFFSET=0x4c415000U
mv main pal_bench64L2
make clean

