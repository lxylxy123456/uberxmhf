## On other machine (I am using Fedora)

### Network configuration
* IPv4 manual `192.168.254.1` mask `255.255.255.0`
* NAT all traffic
```sh
sudo iptables -t nat -A POSTROUTING -s 192.168.254.254 -j MASQUERADE
```

### Install software
* `sudo dnf install amtterm`

## On HP machine

### Network configuration
* IPv4 manual `192.168.254.254` mask `255.255.255.0` route `192.168.254.1`

### Install software
* Edit `/etc/apt/sources.list`, use
  `http://old-releases.ubuntu.com/ubuntu/dists/`
* Install using `apt-get`

```sh
apt-get install \
				openssh-server tmux vim git aptitude \
				pbuilder texinfo ruby build-essential autoconf libtool \
				-y
```

### sudo
Current user is `dev`

```sh
echo 'dev ALL=NOPASSWD:ALL' | sudo tee -a /etc/sudoers
```

Problem: SSH to HP laptop is slow, stuck at `SSH2_MSG_SERVICE_ACCEPT received`
Ref: <https://blog.csdn.net/qq_40907977/article/details/105409810>
Solution: `UseDNS no` in `/etc/ssh/sshd_config`

```sh
echo UseDNS no | sudo tee -a /etc/ssh/sshd_config
```

### SSH
In controlling machine, add option `PubkeyAcceptedKeyTypes +ssh-rsa`

### Install non-PAE kernel

```sh
apt-get install linux-generic
```

### Build old XMHF
Follow `xmhf-v0.2.2/xmhf/doc/building-xmhf.md`
```sh
cd xmhf-v0.2.2
./autogen.sh
./configure --with-approot=hypapps/trustvisor
make
sudo make install
```

#### Build Hello World (avoid DRT)
```sh
./configure --with-approot=hypapps/helloworld --enable-drt=no
```

#### Cross-compile 32-bit TrustVisor on Debian 64-bit

This is just a workaround
```sh
./configure --with-approot=hypapps/trustvisor --enable-drt=no CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld
```

### Build XMHF64

Apt get install `crossbuild-essential-i386`. This is needed to build x64 XMHF
on x64 Debian (because the bootloader is 32-bits).

Compile x64 XMHF on x64 Debian:
```sh
./configure --with-approot=hypapps/trustvisor --with-target-hwplatform=x86_64
```

Cross compile x86 XMHF on x64 Debian:
```sh
./configure --with-approot=hypapps/trustvisor CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld
```

Other useful flags:
* `--disable-drt`: Do not use DRT (e.g. Intel TXT)
* `--enable-debug-symbols`: Generate debug symbol (add `-g` when compiling)
* `--enable-debug-qemu`: Disable some VMCS fields unused by XMHF to support QEMU

#### Installed files
See `xmhf-v0.2.2/Makefile.in`'s `install-bin` target

* `/boot/hypervisor-x86.bin.gz`
* `/boot/init-x86.bin`

### Install XMHF
Follow `xmhf-v0.2.2/xmhf/doc/installing-xmhf.md`
* Ref: <https://github.com/superymk/docs/blob/main/uXMHF/XMHFv0.2.2.md>
* Ref: <https://help.ubuntu.com/community/Grub2/CustomMenus>
* Ref:
<https://www.gnu.org/software/grub/manual/grub/html_node/Naming-convention.html>
* Ref: <https://www.gnu.org/software/grub/manual/legacy/grub.html>
       "11.3 How to specify block lists"

#### Download `i5_i7_DUAL_SINIT_51.BIN`
Download `i5-i7-dual-sinit-51.zip` from
<http://software.intel.com/en-us/articles/intel-trusted-execution-technology/>,
then unzip `i5_i7_DUAL_SINIT_51.BIN` to `/boot`

#### Configure GRUB
Create a custom grub entry in `/etc/grub.d/42_xmhf`, using `40_custom` as a
template

```sh
#!/bin/sh
exec tail -n +3 $0
# This file provides an easy way to add custom menu entries.  Simply type the
# menu entries you want to add after this comment.  Be careful not to change
# the 'exec tail' line above.

menuentry "XMHF" {
	set root='(hd0,msdos1)'		# point to file system for /
	set kernel='/boot/init-x86.bin'
	set boot_drive='0x80'	# should point to where grub is installed
	echo "Loading ${kernel}..."
	multiboot ${kernel} serial=115200,8n1,0x5080 boot_drive=${boot_drive}
	module /boot/hypervisor-x86.bin.gz
	module --nounzip (hd0)+1	# should point to where grub is installed
	module /boot/i5_i7_DUAL_SINIT_51.BIN
}
```

There are a few variables in this script
* `(hd0,msdos1)` should be the place where XMHF files are installed
* In x86-64 XMHF,
	* `/boot/init-x86.bin` becomes `/boot/init-x86_64.bin`
	* `/boot/hypervisor-x86.bin.gz` becomes `/boot/hypervisor-x86_64.bin.gz`
* `0x5080` is serial port's I/O port address. in QEMU this is `0x3f8`
* `(hd0)+1` and `0x80` points to where GRUB is installed (default is like sda)
	* If GRUB is installed on second HDD (sdb), should be `(hd1)+1` and `0x81`

Use GRUB text mode (Ref: <https://superuser.com/a/462096>)

Append the following to `/etc/default/grub`
```
GRUB_TERMINAL=console
GRUB_GFXPAYLOAD_LINUX=text
```

Update grub
```sh
sudo chmod +x /etc/grub.d/42_xmhf
sudo sed -i 's/GRUB_HIDDEN_TIMEOUT=0/GRUB_HIDDEN_TIMEOUT=5/' /etc/default/grub
sudo update-grub
```

While booting, press Shift when booting to wake up grub menu.

##### Reverse Engineer GRUB
* Ref: <https://0xax.github.io/grub/>
`0x68 - 0x1b8` of `/boot/grub/boot.img` is the same as the first secrot of disk.
`/boot/grub/core.img` is almost the same as the second ... sector of disk.

### BIOS options for TrustVisor
Follow `xmhf-v0.2.2/xmhf/doc/hardware-requirements.md` (a little bit)

Ref: <http://h10032.www1.hp.com/ctg/Manual/c05807442>

* Enter BIOS using F10
* Security -> TPM Embedded Security
	* Check "-Reset of TPM from OS"
* System Configurations -> Device Configurations
	* Disable "TXT Technology"
* Security -> TPM Embedded Security
	* Reset to "Factory Defaults"
* Restart, press F1 to restore to factory defaults
* Security -> TPM Embedded Security
	* Enable ...
* Restart, press F1 to enable TPM
* System Configurations -> Device Configurations
	* Enable "TXT Technology"

### Install TrustVisor
Follow `xmhf-v0.2.2/xmhf/doc/installing-xmhf.md`

Add a line in `/etc/modprobe.d/blacklist.conf`:
```
# For TrustVisor
blacklist tpm_infineon
```

(`trousers` not running, skip)

Maybe need to install `jtss` (not covered in tutorial)
<https://sourceforge.net/projects/trustedjava/files/>
* Go to "jTSS (Full Java (tm) TSS)", I used version 0.6
* Can find `jtss_0.6_all.deb` from `jTSS_0.6.zip`

```sh
sudo dpkg -i jtss_0.6_all.deb
# Ignore warning about dependencies
```

Install Java
```sh
sudo apt-get install openjdk-7-jre-headless
```

Install `jTpmTools` version 0.6 from
<https://sourceforge.net/projects/trustedjava/files/jTPM%20Tools/>

```sh
sudo dpkg -i jtpmtools_0.6.deb
# Ignore warning about dependencies

# Fix dependencies
sudo apt-get -f install
```

Take ownership of the TPM (if failed, reset TPM in BIOS)

```sh
jtt take_owner -e ASCII -o PASSWORD_2
```

Define NV spaces

```
jtt nv_definespace \
    --index 0x00015213 \
    --size 20 \
    -o 'PASSWORD_2' \
    -e ASCII \
    -p 11,12 \
    -w \
    --permission 0x00000000 \
    --writelocality 2 \
    --readlocality 2
jtt nv_definespace \
    --index 0x00014e56 \
    --size 32 \
    -o 'PASSWORD_2' \
    -e ASCII \
    -p 11,12 \
    -w \
    --permission 0x00000000 \
    --writelocality 2 \
    --readlocality 2
```

Get the following error, give up
```
  ---------------------
   IAIK Java TPM Tools 
  ---------------------


14:04:22:182 [INFO] NvDefineSpace::execute (146):	Defining NV space to depend on pcr: 11
14:04:22:440 [INFO] NvDefineSpace::execute (146):	Defining NV space to depend on pcr: 12
iaik.tc.tss.api.exceptions.tcs.TcTcsException: 
TSS Error:
error layer:                0x3000 (TSP)
error code (without layer): 0xcd
error code (full):          0x30cd
error message: Bad memory index
additional info: index already defined

	at iaik.tc.tss.impl.java.tsp.TcNvRam.defineSpace(TcNvRam.java:109)
	at iaik.tc.apps.jtt.tboot.NvDefineSpace.execute(NvDefineSpace.java:240)
	at iaik.tc.utils.cmdline.SubCommand.run(SubCommand.java:69)
	at iaik.tc.utils.cmdline.SubCommandParser.parse(SubCommandParser.java:41)
	at iaik.tc.apps.JTpmTools.main(JTpmTools.java:198)

14:04:22:642 [ERROR] JTpmTools::main (209):	application exits with error: 
TSS Error:
error layer:                0x3000 (TSP)
error code (without layer): 0xcd
error code (full):          0x30cd
error message: Bad memory index
additional info: index already defined
 (return: -1)
```

### Set AMT
Follow <https://github.com/superymk/docs/blob/main/IntelAMT/amt.md>

Ctrl+P during boot
* Old password is `admin`, set new password to `PASSWORD_1`
* Configure network (IPv4)
* Select activate network
* In AMT configuration, enable SOL and legacy redirection mode

In Ubuntu (IP is `192.168.254.254`),
```sh
dmesg | grep ttyS
# [    1.762170] 0000:00:16.3: ttyS4 at I/O 0x5080 (irq = 17) is a 16550A
```

In own machine,
```sh
amtterm -p 'PASSWORD_1' 192.168.254.254
```

Can see from wireshark / tcpdump that port 16994 on Ubuntu is connected.
```sh
tcpdump -lnnq -i any host 192.168.254.254 and port 16994
```

When system restart, see messages.

### Running PAL

In `xmhf-v0.2.2/hypapps/trustvisor/trustvisor-api`, remove unnecessary code,
run `make`

* Without XMHF: get SIGILL
* With XMHF: get "ret:0x0", successful
* Call `init_tz_sess()`: strange error

### Build PAL using TEE-SDK
* Follow `xmhf-v0.2.2/xmhf/doc/building-xmhf.md`
* Follow `xmhf-v0.2.2/hypapps/trustvisor/doc/building-trustvisor.md`
* Follow `hypapps/trustvisor/tee-sdk/doc/installing-sdk.md`
* Follow `hypapps/trustvisor/tee-sdk/doc/using-sdk.md`

Build XMHF first
...

Dependencies
```sh
sudo apt-get -y install pkg-config
```

Install-dev, etc
```sh
sudo make install-dev
./configure --with-approot=hypapps/trustvisor --prefix=/usr/local/i586-tsvc/usr
sudo make install-dev
pushd hypapps/trustvisor/tee-sdk/ports/newlib
wget ftp://sourceware.org/pub/newlib/newlib-1.19.0.tar.gz
tar xf newlib-1.19.0.tar.gz
pushd newlib-1.19.0
patch -p1 < ../newlib-tee-sdk-131021.patch
popd
popd
pushd hypapps/trustvisor/tee-sdk/ports/openssl
wget https://www.openssl.org/source/old/1.0.0/openssl-1.0.0d.tar.gz
tar xf openssl-1.0.0d.tar.gz
pushd openssl-1.0.0d
patch -p1 < ../openssl-tee-sdk-131021.patch
popd
popd
```

Build TEE-SDK
```sh
cd hypapps/trustvisor/tee-sdk

## Fix bug if using Debian 11
#echo AM_PROG_AR | tee -a tz/configure.ac
#echo 'AR = ar' | tee -a tz/dev/tv/src/Makefile.am
#echo 'AR = ar' | tee -a tz/dev/null/src/Makefile.am
#echo 'AR = ar' | tee -a tz/src/Makefile.am
#sed -i 's/install: all install_docs install_sw/install: all install_sw/' \
#	ports/openssl/openssl-1.0.0d/Makefile.org
## End fixing bug

sudo make all openssl
```

Build test (failed)
```sh
cd examples/test
PKG_CONFIG_LIBDIR=/usr/local/i586-tsvc/usr/lib/pkgconfig make
# Failed with: new lib does not remove global symbol _GLOBAL_OFFSET_TABLE_
cd ..
```

Build newlib (succeed)
```sh
cd examples/newlib
make
./hello
cd ..
```


## Changing environment
* Disable TXT in BIOS
	* Receive error `FATAL ERROR: TXT not suppported`
* TXT requires TPM in BIOS (so cannot just disable TPM)
* Disable TXT and TPM in BIOS
	* Receive error `FATAL ERROR: TXT not suppported`
* Using uberXMHF's newer code
	* Not a lot of change; can run as expected
* Disabling DRT when compiling XMHF (`--enable-drt=no`)
	* Receive error `FATAL ERROR: Could not access HW TPM`
* Disable DRT and use helloworld (a simpler hypapp)
	* Successful (can disable TXT in BIOS, but still need to load SINIT in grub)
* (If have time) Try Debian Stretch (latest version supporting non-PAE)
	* Should also support stable, see `linux-image-686`
* Try HelloWorld hypapp and QEMU
	* See [debian.md](./debian.md), not successful
* Boot Ubuntu with TrustVisor compiled on Debian (higher gcc version)
	* Successful
* Boot Debian with TrustVisor on HP
	* unhandled exception, see [debian.md](./debian.md)

## Miscellaneous

### Results
* Boot without XMHF: 3084640256 bytes memory
* Boot with XMHF, TrustVisor: 2923159552 bytes memory
* Boot with XMHF, HelloWorld: 2927353856 bytes memory

### Guideline for removing TPM
* XMHF: DRTM compile off, turn off GETSEC
* TrustVisor: disable uTPM
* Disable VT-d
* Intel TXT contains DRTM

### Compile options
In `configure.ac`, see `enable_drt`, use `./configure ... --enable-drt=no`

### Implementation
To prevent difficulties in verification
* Avoid using linked lists
* Avoid dynamically changing function pointers

### Git version
* XMHF version 0.2.2 can be found in <https://github.com/uberspark/uberxmhf>'s
  git history, tag `v0.2.2`
	* Only `Makefile.in` hardcoded version info; not a big deal
* Current version of xmhf does not have a lot of change
	* Compared using `diff --recursive`

### Compile history

|Date & dir name     | HV size | Note                                   |
|--------------------|---------|----------------------------------------|
| 20211023xmhf022    | 308719  | First compile v0.2.2                   |
| 20211025uberxmhf   | 308660  | First compile latest                   |
| 20211025no-drt     | 307965  | Latest, disable DRT                    |
| 20211026hw         | 213222  | Latest, hello world                    |
| 20211026hw-ndrt    | 212458  | hello world w/o DRT                    |
| 20211027deb        | 306701  | Compile in Debian (higher gcc version) |

### Compile environment using docker
Use docker image `ubuntu:12.04`

```sh
URL1=http://archive.ubuntu.com/ubuntu/
URL2=http://old-releases.ubuntu.com/ubuntu/
sed -i "s|$URL1|$URL2|" /etc/apt/sources.list
apt-get update
apt-get install -y nano sudo
apt-get install -y ... (from above)
git clone https://github.com/lxylxy123456/uberxmhf -b master --depth 1
```

Cannot use docker because docker is x64, give up

### Compile environment using QEMU
Image:
<https://old-releases.ubuntu.com/releases/12.04/ubuntu-12.04.1-desktop-i386.iso>

* Manually update `/etc/apt/sources.list`
* Install `openssh-server`
* Follow other steps

