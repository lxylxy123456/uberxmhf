# Files used to configure target (machine that runs XMHF)

* `install86.sh`: goes to `~`, install i386 XMHF from `/tmp` to `/boot`
* `install64.sh`: goes to `~`, install amd64 XMHF from `/tmp` to `/boot`
* `fedora-install86.sh`: Fedora QEMU version of `install86.sh`
* `fedora-install64.sh`: Fedora QEMU version of `install64.sh`
* `42_xmhf_i386`: goes to `/etc/grub.d/`, configure GRUB for x86 XMHF
* `43_xmhf_amd64`: goes to `/etc/grub.d/`, configure GRUB for amd64 XMHF
	* Make sure to change serial configuration; QEMU should be `0x5080 -> 0x3f8`
* `bash_aliases`: goes to `~/.bash_aliases`
