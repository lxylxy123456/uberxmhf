#!/bin/bash
sudo install --mode=644 /tmp/{hypervisor-x86-amd64.bin.gz,init-x86-amd64.bin} /boot/boot
sudo grub2-editenv /boot/grub2/grubenv set next_entry="XMHF-amd64"
