# XMHF64 Evaluation

## Scope
* All configurations

## Behavior
We need to evaluate XMHF64's performance for the research

## Debugging

### XMHF Versions

We use latest version `xmhf64 f6c71ded5` as XMHF64, we use `v6.1.0 3cd28bfe5`
as XMHF.

For old XMHF:
* change 3 places to `#define EU_LOG_LVL EU_ERR`. Similar to `fd47fbe44`.
* Modify `xmhf/src/libbaremetal/libtv_utpm/utpm.c` to remove printf
  "utpm_extend: extending PCR 0"
For XMHF64: remove the call to `xmhf_debug_arch_putc` in `emhfc_putchar`. See
`xmhf64-dev d0e5be5d8` for an example.

old XMHF compile:
```sh
# GCC 4.6.3
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' && make
```

new XMHF compile:
```sh
# xmhf32O0
# GCC 12.2.1
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--with-target-subarch=i386' && make -j 8
```

new XMHF compile with O3:
```sh
# xmhf32O3
# GCC 12.2.1
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--with-target-subarch=i386' '--with-opt=-O3 -Wno-array-bounds' && make -j 8
```

new XMHF64:
```
# xmhf64O3
TODO: max memory
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--with-target-subarch=amd64' '--with-amd64-max-phys-addr=0x140000000' '--enable-nested-virtualization' '--with-opt=-O3 -Wno-array-bounds' && make -j 8
```

XMHF64 compile

TODO: calling convention changed, pal_demo does not work on legacy XMHF

### CPU frequency

```sh
for i in /sys/devices/system/cpu/cpu*/cpufreq/; do
	cat "$i/scaling_max_freq" | sudo tee "$i/scaling_min_freq"
done
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_m*_freq
sudo grep . /sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_cur_freq
```

### Benchmarks

#### sysbench 0.4.12

Easily installed on Ubuntu 12.04.

```sh
sysbench --test=cpu run
```

Deprecated because we want to use later version

#### sysbench 1.0.20

Download from GitHub / easily installed on Fedora.

Compile:
```sh
# GCC 4.6.3
cd sysbench-1.0.20
./autogen.sh
./configure --without-mysql
make -j 4
PATH="$PWD/src:$PATH"
```

Run:
```sh
# see "total number of events"
sysbench cpu run
sysbench cpu run --threads=2
sysbench cpu run --threads=3
sysbench cpu run --threads=4
sysbench memory run
sysbench memory run --threads=2
sysbench memory run --threads=3
sysbench memory run --threads=4
sysbench fileio prepare				# for each run
sysbench fileio run --file-test-mode=seqwr
sysbench fileio cleanup				# for each run
sysbench fileio run --file-test-mode=seqrewr
sysbench fileio run --file-test-mode=seqrd
sysbench fileio run --file-test-mode=rndrd
sysbench fileio run --file-test-mode=rndwr
sysbench fileio run --file-test-mode=rndrw
```

Use `sysbench.sh` to run automatically

### LMbench 3.0-a9

Download from sourceforge, cannot compile. See
```
bench.h:39:17: fatal error: rpc/rpc.h: No such file or directory
```

Debian 11: add `non-free` to `/etc/apt/sources.list`, then can install easily

Run with `sudo lmbench-run`, answer some questions, ...

Too complicated, probably deprecate

#### PAL
Write own benchmark (called `pal_bench`) in `xmhf64-dev 39ab6174c`. Run with
`palbench.sh`.

### Test result for XMHF vs XMHF64

See `ubuntu.csv` and `ubuntu.7z`

Legend
```
log: full sysbench log
result: sysbench table with number of events
plog: full pal_bench log
pal: pal_bench results

u1232: Ubuntu 12.04.1 LTS, i686, Linux 3.2.0-150-generic
oldxmhf32O0: old XMHF, 32-bit, gcc -O0
xmhf32O0: XMHF64, 32-bit, gcc -O0
xmhf32O3: XMHF64, 32-bit, gcc -O3
```

Commands:
```sh
cd ~/pal-tmp/; rm *; script -c '../palbench.sh ../pal_bench32 "old XMHF i386 O0, Ubuntu 12.04 x86"' plog

cd ~/sysbench-tmp/; rm *; script -c '../sysbench.sh "XMHF64 i386 O0, Ubuntu 12.04 x86"' log
cd ~/pal-tmp/; rm *; script -c '../palbench.sh ../pal_bench64 "XMHF64 i386 O0, Ubuntu 12.04 x86"' plog

cd ~/sysbench-tmp/; rm *; script -c '../sysbench.sh "XMHF64 i386 O3, Ubuntu 12.04 x86"' log
cd ~/pal-tmp/; rm *; script -c '../palbench.sh ../pal_bench64 "XMHF64 i386 O3, Ubuntu 12.04 x86"' plog
```

### Testing for nested virtualization

Install Debian using KVM with
```
./bios-qemu.sh --q35 -d a.qcow2 '' -c debian-11.0.0-amd64-netinst.iso
...
./bios-qemu.sh --q35 -d a.qcow2 +1 --ssh 1234 -smp 2
qemu-img convert -f qcow2 -O vdi a.qcow2 a.vdi
qemu-img convert -f qcow2 -O vmdk a.qcow2 a.vmdk
```

Install sysbench, copy `palbench.sh`, `sysbench.sh`, and `pal_bench64L2`.
`mkdir ben`.

Note: at least in KVM, cannot set CPU scaling.

In VMware, port forwarding is configured in
`/usr/lib/vmware/configurator/vmnet-nat.conf`. Add under `[incomingtcp]` TODO
Ref:
* <https://stackoverflow.com/questions/52386841/>
* <https://superuser.com/questions/571196/>

```
COMMENT="XMHF64 amd64 O3, KVM, Debian 11 x64"
cd ben
rm *
script -c "../palbench.sh ../pal_bench64 $COMMENT" plog
script -c "../sysbench.sh $COMMENT" log
```

Legend
```
d1164: Debian 11, x64, Linux 5.10.0-21-amd64
kvm: KVM, version same as Linux
vmware: VMware Workstation 16.2.4
virtualbox: VirtualBox 6.1.42
```

### SLOCCount

Using sloccount to measure code size, wrote `remove_nv.py` to remove nested
virtualization code.

Result in `sloccount.csv` and `sloccount/*.txt`. Approximate to reproduce:

```sh
cd .../uberxmhf/xmhf/xmhf/src/xmhf-core/xmhf-runtime
sloccount . > .../uberxmhf.txt
cd .../xmhf64/xmhf/src/xmhf-core/xmhf-runtime
sloccount . > .../xmhf64.txt
cd ...
python3 ../notes/bug_114/remove_nv.py
cd -
rm -r xmhf-nested
sloccount . > .../xmhf64_nonested.txt
```

### Retry testing nested virtualization

We need to make sure the inner-most guest have the same configuration across
tests. We install another Debian on VirtualBox, with swap disabled, no
graphical user interface. We call this "debian(light)". "OS" is the normal OS
we use.

Other things to install:
```
apt-get install sudo sysbench qemu-system-i386
```

Configurations to test

* L0
	* debian(light) (native for L1)
* L1
	* xmhf debian(light)
	* OS virtualbox debian(light)
	* OS vmware debian(light)
	* OS kvm debian(light) (native for L2)
* L2
	* xmhf OS kvm debian(light)
	* OS kvm debian(light) kvm debian(light)
	* OS vmware debian(light) kvm debian(light)
	* OS virtualbox debian(light) kvm debian(light)

Linux command line
	* Use `mem=1g` to limit memory for L0
	* Use `vga=normal nofb nomodeset video=vesafb:off i915.modeset=0` to force
	  Linux to use 80x25 VGA. Maybe useful with QEMU's `-display curses`
		* Ref: <https://ubuntuforums.org/archive/index.php/t-2225544.html>

