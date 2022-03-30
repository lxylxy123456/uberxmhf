#!/bin/bash
sudo install --mode=644 /tmp/{hypervisor-x86-i386.bin.gz,init-x86-i386.bin} /boot/boot
sudo grub2-editenv /boot/grub2/grubenv set next_entry="XMHF-i386"
