# Use debugfs to install XMHF in CI

## Scope
* All XMHF
* Git `007ab17d2`

## Behavior

Continuous integration tests are slow because the guest needs to be booted to
install XMHF. We did not mount the guest's disk in the host for security
reasons (as mount needs sudo).

Looks like there are ways to modify e2fs, such as debugfs. We can use it to
modify the file system and install XMHF.

## Debugging

In Debian, debugfs is already installed in `/sbin/debugfs` (package `e2fsprogs`)

The idea is to start from a known Debian GRUB installation.

Proof-of-concept commands
```sh
cd /tmp

dd if=/dev/zero of=a.img bs=1M seek=128 count=0
echo $'n\n\n\n\n\nw\n' | /sbin/fdisk a.img
echo $'p\n' | /sbin/fdisk a.img

dd if=/dev/zero of=b.img bs=1M seek=127 count=0
/sbin/mkfs.ext4 b.img

/sbin/debugfs -w b.img
	mkdir boot
	write <host file> <file in b.img>
```

### Modifying Debian

We modify a Debian in QEMU to get the GRUB installed on it.

#### Change UUID

We want to change UUID of the root directory

First change its UUID. However, this requires unmounting the disk. So we need
two Debians: one to perform the edits and one to be changed.

```sh
DEV=/dev/sdb1

sudo e2fsck -f $DEV
sudo tune2fs -U random $DEV
sudo blkid $DEV

sudo mount $DEV /mnt
for i in /dev /dev/pts /proc /sys /run; do sudo mount -B $i /mnt/$i; done
sudo chroot /mnt
grub-install /dev/sdb
update-grub

# restart and boot $DEV
```

However, it is not very successful, because Linux will break and root is
mounted as read-only. So give up.

#### Test GRUB

We believe that we just need to copy everything in `/boot`, and GRUB should
still work.

```sh
DEV=/dev/sda1

yes | sudo mkfs.ext4 $DEV
sudo mount $DEV /mnt
sudo cp -a /boot/ /mnt/boot/

# reboot
```

So it looks like the tricky part is only the first MB of MBR. Then other files
can just be copied in the EXT4 partition.

#### Try to reconstruct things

Now we no longer need 2 Debians.

```sh
cd /tmp

# Update grub
sudo sed -i -e "s/boot_drive='0x80'/boot_drive='0x81'/" \
			-e "s/(hd0)+1/(hd1)+1/" \
			/etc/grub.d/42_xmhf_i386 /etc/grub.d/43_xmhf_amd64
sudo update-grub

# Collect data
sudo dd if=/dev/sda of=a.img bs=1M count=1
cp -r /boot .
sudo grub-editenv /boot/grub/grubenv set next_entry="XMHF-amd64"
cp /boot/grub/grubenv grubenv-amd64
sudo grub-editenv /boot/grub/grubenv set next_entry="XMHF-i386"
cp /boot/grub/grubenv grubenv-i386
```

The data collected above is saved in this repo:
`a.img  boot  grubenv-amd64  grubenv-i386`

To reduce size, we can remove `/boot/grub/{fonts,locale,unicode.pf2}`, because
graphical mode is not used.

Then we can construct the image. These commands work on both Debian and Fedora.

```
# Construct ext4
dd if=/dev/zero of=b.img bs=1M seek=127 count=0
/sbin/mkfs.ext4 b.img
echo mkdir boot | /sbin/debugfs -w b.img
find boot/grub -type d -exec echo mkdir {} \; | /sbin/debugfs -w b.img
find boot/grub -type f -exec echo write {} {} \; | /sbin/debugfs -w b.img

# Install XMHF
SUBARCH=amd64
for i in hypervisor-x86-${SUBARCH}.bin.gz init-x86-${SUBARCH}.bin; do
    echo write boot/$i boot/$i
done | /sbin/debugfs -w b.img
echo rm boot/grub/grubenv | /sbin/debugfs -w b.img
echo write grubenv-${SUBARCH} boot/grub/grubenv | /sbin/debugfs -w b.img

# Concat to c.img
dd if=/dev/zero of=c.img bs=1M seek=128 count=0
dd if=a.img of=c.img conv=sparse,notrunc bs=1M count=1 
dd if=b.img of=c.img conv=sparse,notrunc bs=1M seek=1
```

Wrote `.jenkins/test3.py` to implement this idea. Jenkins runs much faster now.

We can further optimize the space by removing unneeded GRUB modules. `lsmod`
in GRUB's shell shows which modules are needed.

Using `-display curses`, can copy characters from QEMU.

```
grub> lsmod
Name    Ref Count       Dependencies
minicmd 1
loadenv 1               disk,extcmd
disk    2
test    1
normal  1               terminal,gettext,bufio,boot,crypto,verifiers,extcmd
gzio    0               gcry_crc
gcry_crc        1               crypto
terminal        2
gettext 2
bufio   2
boot    2
crypto  4
verifiers       2
extcmd  4
biosdisk        1
part_msdos      1
ext2    1               fshelp
fshelp  2
grub> 
```

However, GRUB breaks after including only the the above modules. So give up.

### Circle CI

Unfortunately, Circle CI breaks.

Created new branch `xmhf64-bug_069`. In `4b87ee173`, use `xxd` to dump the
entire content of `c.img`. Looks like that `debugfs` cannot correctly write
files.

After some debugging, looks like debugfs is at an older version, and `write`
command cannot handle paths correctly:
* `write file file` is correct
* `write file path/file` will cause a file called `path/file` created in the
  root directory. This causes I/O error when mounting the file system and ls
  the root directory.

The problem can be fixed by `cd` to the correct directory and then write file.

The last commit of branch `xmhf64-bug_069` is `db4bee8a4`.

## Fix

`007ab17d2..18ecd868e`
* Use debugfs to speed up CI testing

