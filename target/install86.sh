#!/bin/bash
sudo install --mode=644 /tmp/{hypervisor-x86-i386.bin.gz,init-x86-i386.bin} /boot
sudo grub-editenv /boot/grub/grubenv set next_entry="XMHF"
