# CI in Debian

## Scope
* Not applicable

## Behavior
We want to run Jenkins CI in Debian, which will be much faster

## Debugging

Git `2a206b1d0` creates `docs/pc-intel-x86_64/ci.rst`, which is the
documentation for all CI platforms.

### Git clean bug

We found a bug in git. In Debian (git version 2.30.2), running `a.sh` will
not remove `a/asdf`. However, in Fedora (git version 2.35.3 and above), running
`a.sh` will remove them. The result is that in Debian, `_build_*` directories
are not removed, so building amd64 fails.

As a workaround, manually remove the files. See `1b63cf928`.

### 64-bit bug (likely x2APIC)

We see that 32-bit runs well. 64-bit prints a lot of messages, but Linux can
never be reached via ssh.

We remember that a similar problem happens on Circle CI. At that time x2APIC
makes CI not stable. This time the pipeline consistently fails.

In git `a08b3f44f` (branch `xmhf64-dev`), removed x2APIC. Then Jenkins pipeline
succeeds. Now we should try to reproduce this problem without Jenkins. At this
time, we know that a workaround is to simply remove x2APIC.

When trying to reproduce this problem, we run the pipeline with x2APIC enabled.
Then copy the QEMU command. This is reproducible in HP 840.

Then we try to compile XMHF manually, run `gr`, copy `grub/c.img` to HP, run
`./bios-qemu.sh --gdb 1234 -d build64 +1  -d debian11x64 -t`. This also
reproduces the problem.

We can see that the problem happens when CPU 0 is waking up CPU 1. CPU 2 and 3
are sleeping in `while(!vcpu->sipireceived);`. CPU 0 and 1 are stuck at
`0xffffffff818c2e3e`. Looks like waiting for interrupts.

```
   0xffffffff818c2e3c:	sti    
   0xffffffff818c2e3d:	hlt    
=> 0xffffffff818c2e3e:	ret    
```

This bug is also reproducible with only 2 CPUs.

This bug is even reproducible with only 1 CPU.

The Linux's stack is (same for UP and SMP)
```
#0  0xffffffff818c2e3e in native_safe_halt () at arch/x86/include/asm/irqflags.h:61
#1  0xffffffff818c2cea in arch_safe_halt () at arch/x86/include/asm/paravirt.h:150
#2  default_idle () at arch/x86/kernel/process.c:688
#3  0xffffffff818c2f58 in default_idle_call () at kernel/sched/idle.c:112
#4  0xffffffff810c1718 in cpuidle_idle_call () at kernel/sched/idle.c:194
#5  do_idle () at kernel/sched/idle.c:300
#6  0xffffffff810c1939 in cpu_startup_entry (state=state@entry=CPUHP_ONLINE) at kernel/sched/idle.c:396
#7  0xffffffff818b63a4 in rest_init () at init/main.c:721
#8  0xffffffff82c31afd in arch_call_rest_init () at init/main.c:845
#9  0xffffffff82c3209c in start_kernel () at init/main.c:1057
#10 0xffffffff810000f5 in secondary_startup_64 () at /build/linux-ha3l9a/linux-5.10.70/arch/x86/kernel/head_64.S:292
#11 0x0000000000000000 in ?? ()
```

The CPU is idle'ing. EFLAGS.IF is set. So now the question is what is the
CPU waiting for?

In git `c31d23289`, print x2APIC reads and writes. But have no idea from these.

This is going to be complicated, because we need to debug Linux code. For now,
remove x2APIC support.

Ideas:
* Compare XMHF + x2APIC and XMHF + xAPIC behavior
* Compare XMHF + x2APIC and x2APIC behavior
* Since the problem is reproducible in UP, we should be able to debug with GDB
* Use QEMU's `info lapic`
	* Check whether EOIs are sent correctly
* In UP, what happens if enable APIC virtualization?
	* Get mysterious VMRESUME errors, give up

Looks like the problem is related to EOI. In QEMU's `info lapic`, can see that
ISR is `236`, but IRR is `34 48 236`. This means that APIC thinks 236 interrupt
has not returned. See "10.8.4 Interrupt Acceptance for Fixed Interrupts" and
"10.8.5 Signaling Interrupt Servicing Completion"

We compare XMHF + x2APIC and x2APIC. Using the following GDB script to break at
the EOI WRMSR.

```
hb *0xffffffff81062179
c
```

The code is
```
=> 0xffffffff81062179:	wrmsr  
   0xffffffff8106217b:	ret    
```

The break point will hit two times. At the first time `ISR=48, IRR=`. The
expected behavior is:
* User enter `si` in GDB
* RIP becomes `0xffffffff8106217b`, LAPIC is `ISR= IRR=`

However, when XMHF + x2APIC, the behavior is
* User enter `si` in GDB
* RIP becomes `0xffffffff810fae33`, LAPIC is `ISR=48 IRR=48`

If we add a break point at `0xffffffff8106217b`,
* User enter `hb *0xffffffff8106217b` and `c` in GDB
* RIP becomes `0xffffffff8106217b`, LAPIC is `ISR=48, IRR=`

Now we have some ideas of why Linux stucks. I think entering `si` causes RIP
to jump rapidly is because QEMU / KVM enters hypervisor mode.

Now we debug hypervisor code

```
hb *0xffffffff81062179
c
d
hb *0xffffffff8106217b
hb *xmhf_parteventhub_arch_x86vmx_intercept_handler
si
```

We can then manually navigate GDB to the WRMSR in hypervisor. Can see that
ISR is not cleared correctly when EOI is written.

After some debugging, see that if the WRMSR to EOI is repeated, the ISR becomes
correct. I am guessing that this is a bug in QEMU / KVM. Need to debug QEMU.

### Debugging QEMU

We use debuginfod to download things automatically. The first 2 lines may be
unnecessary.

```sh
apt-get install debuginfod
apt-get source qemu-system-x86
export DEBUGINFOD_URLS="https://debuginfod.debian.net"
```

* Ref: <https://wiki.debian.org/HowToGetABacktrace>

In QEMU's GDB
```
handle SIGUSR1 pass nostop noprint
set pagination off
set confirm off
r -m 512M -gdb tcp::2198 -smp 1 -cpu Haswell,vmx=yes -enable-kvm -serial stdio -drive media=disk,file=.../c+1.qcow2,index=1 -drive media=disk,file=.../debian11x64-t.qcow2,index=2
```

In guest's (XMHF / Linux) GDB
```
hb *0xffffffff81062179
c
d
hb *0xffffffff8106217b
hb *xmhf_parteventhub_arch_x86vmx_intercept_handler
si
hb *_vmx_handle_intercept_wrmsr + 928
c
```

In QEMU's GDB, Ctrl + C and

```
hb kvm_get_apic_state
c
```

In guest's (XMHF / Linux) GDB

```
si
```

In QEMU's GDB, can see call stack
```
#0  0x0000555555b50130 in kvm_get_apic_state ()
#1  0x0000555555b36cdf in  ()
#2  0x0000555555b3cbc5 in kvm_arch_get_registers ()
#3  0x0000555555be8329 in  ()
#4  0x0000555555b1d59c in process_queued_cpu_work ()
#5  0x0000555555c6a5b0 in  ()
#6  0x0000555555e03669 in  ()
#7  0x00007ffff6a4dea7 in start_thread (arg=<optimized out>) at pthread_create.c:477
#8  0x00007ffff697ddef in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

However, looks like debuginfod is not working well. So we compile QEMU
ourselves. After compiling, debugging is much easier.

Looks like `kapic` contains APIC information in binary format.
`kvm_get_apic_state()` converts `kapic` to `s`, which is more readable.
Now we try to break at changes to `kapic`

```
(gdb) p kapic
$12 = (struct kvm_lapic_state *) 0x7ffff5695430
(gdb) p/x s->irr
$13 = {0x0, 0x10000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
(gdb) p/x s->isr
$14 = {0x0, 0x10000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
(gdb) 
```

However, I think it is very possible that `kapic` is modified by KVM directly.
So we read Linux source code.

In `arch/x86/kvm/lapic.c`, function `kvm_x2apic_msr_write()`, can see relevant
information. The call stack should be

```
kvm_x2apic_msr_write
 kvm_lapic_reg_write
  apic_set_eoi
   apic_find_highest_isr
   apic_clear_isr
```

However, we need to debug Linux in order to get further information.

### Debugging KVM

We use KVM nested virtualization to debug KVM. Use ftp to synchronize files.
The setup is in `bug_064`. We need to load `kvm.ko` instead (not `vmlinux`).

Also, the kernel module does not know its load address at compile time, so in
Linux, run `sudo cat /sys/module/kvm/sections/.text`. I see
`0xffffffffc05d4000`.

GDB script becomes

```
set pagination off
set confirm off
symbol-file -o 0xffffffffc05d4000 usr/lib/debug/lib/modules/5.10.0-10-amd64/kernel/arch/x86/kvm/kvm.ko
directory linux-5.10.84/

hb kvm_x2apic_msr_write
c
```

However, this does not work. It looks like the `symbol-file` command's offset
is still wrong. On the Internet people say that `lx-symbols` can be used, but
it requires compiling Linux with `CONFIG_GDB_SCRIPTS=y`.
`/boot/config-5.10.0-10-amd64` says that `CONFIG_GDB_SCRIPTS` is not set.

This is becomming a big project. The next step may be compiling Linux, maybe
with kvm built in (not as a module).

### Compiling Linux, try 2

In `bug_076`, we tried to build Linux, but failed. Now we try again. Instead of
copying the config from `/boot/config-5.10.0-14-amd64`, we start from scratch.

In "Virtualization", set "KVM support" etc from "M" to "*".

Unfortunately, building from vanilla kernel still fails. Now try to build from
kernels from Debian.

* Ref: <https://github.com/lxylxy123456/ECS150Demo/blob/master/CompileLinux.md>

```
sudo apt-get install linux-source-5.10
tar xaf /usr/src/linux-source-5.10.tar.xz 
cd linux-source-5.10/
make nconfig
make clean
time make deb-pkg -j `nproc`
```

However, we still see an error

Adding `KBUILD_VERBOSE=1` to make command line will printing the commands used
when making.

The build message ends with

```
make[2]: *** [debian/rules:7: build-arch] Error 2
dpkg-buildpackage: error: debian/rules binary subprocess returned exit status 2
make[1]: *** [scripts/Makefile.package:77: deb-pkg] Error 2
make: *** [Makefile:1574: deb-pkg] Error 2
```

We run the `debian/rules build-arch` in shell, and see

```
$ debian/rules build-arch
/usr/bin/make KERNELRELEASE=5.10.113 ARCH=x86 	KBUILD_BUILD_VERSION=1 -f ./Makefile
make[1]: Entering directory '/home/dev/linux/linux-source-5.10'
  CALL    scripts/checksyscalls.sh
  CALL    scripts/atomic/check-atomics.sh
  DESCEND  objtool
  DESCEND  bpf/resolve_btfids
  CHK     include/generated/compile.h
make[2]: *** No rule to make target 'debian/certs/debian-uefi-certs.pem', needed by 'certs/x509_certificate_list'.  Stop.
make[1]: *** [Makefile:1846: certs] Error 2
make[1]: Leaving directory '/home/dev/linux/linux-source-5.10'
make: *** [debian/rules:7: build-arch] Error 2
$ 
```

Following <https://unix.stackexchange.com/a/646758>, in `make nconfig`, go to
* Cryptographic API
* Certificates for signature checking (last row)
* Provide system-wide ring of trusted keys
* Additional X.509 keys for default system keyring
* Remove the `debian/certs/debian-uefi-certs.pem` with empty string

### Hide x2APIC

For now, removed x2APIC from CI. See git `356013eb7..78b56c9ca`. A
configuration option is added.

### Compile Linux, try 3

The process of building Linux is currently
```
wget https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/linux-5.17.9.tar.xz
tar xaf linux-5.17.9.tar.xz
cd linux-5.17.9/
make nconfig
	# Virtualization: Set KVM to build within kernel (not as a module)
	# Cryptographic API: Remove key ring
make clean
time make deb-pkg -j `nproc`
```

The compilation takes a lot of time and storage space. It took 50 mins and 24G
on HP 840. After compiling, `../linux-image-5.17.9_5.17.9-1_amd64.deb` can be
used to install the new kernel.

The setup is something like
```
python3 -m pyftpdlib -u lxy -P $PASSWORD -i $IP -p $PORT
# (another shell)
cd ../..
./bios-qemu.sh --gdb 1235 -d debian11x64-q -t -smp 1 --ssh 22 --fwd 1234 1234
# (ssh ...)
# Install self-compiled Linux
cp mnt/linux/linux-image-5.17.9_5.17.9-1_amd64.deb .
sudo apt-get install ./linux-image-5.17.9_5.17.9-1_amd64.deb
init 6
mkdir mnt
curlftpfs $IP:$PORT mnt -o user=lxy:$PASSWORD
ln -s mnt/debian11x64.qcow2 .
mnt/bios-qemu.sh -d debian11x64.qcow2 +t --ssh 22 -smp 2 --gdb 1234
```

### Debug Linux

When running in nested KVM with latest Linux kernel, this bug is reproducible
```
HP 840 (Debian 11, 5.10.0-14-amd64)
	KVM
		QEMU (Debian 11, 5.17.9, nokaslr)
			KVM
				XMHF (amd64)
					Debian (use x2APIC, Debian 11, 5.10.0-9-amd64, nokaslr)
```

Make sure to turn off KASLR for the virtual machines.

The new GDB procedure is (for `xmhf64-dev` branch, git commit `01a4ab597`):

First break at before the first hypervisor WRMSR (GDB connect to inner KVM).

```
hb *0xffffffff81062179
c
d
hb *0xffffffff8106217b
hb *xmhf_parteventhub_arch_x86vmx_intercept_handler
si
hb *_vmx_handle_intercept_wrmsr + 807
c
```

Then cd to directory that compiles Linux, and connect to outer KVM's gdb

```
set pagination off
set confirm off
symbol-file ./debian/linux-image-dbg/usr/lib/debug/vmlinux-5.17.9
hb kvm_lapic_reg_write
c
```

After setting this break point, `c` in inner KVM. Unfortunately, outer KVM
crashes with the familiar bug
```
qemu-system-x86_64: ../../target/i386/kvm.c:634: kvm_queue_exception: Assertion `!env->exception_has_payload' failed.
```

We probably need print debugging for KVM.

### Recompile KVM

Since we are only debugging KVM, we run the KVM to be debugged on the physical
machine (no KVM nested in KVM). There are many sources online saying that it
is possible to compile only a kernel module quickly.

```sh
tar xf /usr/src/linux-source-5.10.tar.xz
cd linux-source-5.10/
cp /boot/config-$(uname -r) .config
make oldconfig
make nconfig
	# nothing to do
make M=arch/x86/kvm
```

However, when running the last command, see error 

```
$ make M=arch/x86/kvm
make[1]: *** No rule to make target 'arch/x86/kvm/../../../virt/kvm/kvm_main.o', needed by 'arch/x86/kvm/kvm.o'.  Stop.
make: *** [Makefile:1846: arch/x86/kvm] Error 2
$ 
```

Refs
* <https://yoursunny.com/t/2018/one-kernel-module/>
* <https://www.cyberciti.biz/tips/compiling-linux-kernel-module.html>
* <https://yulistic.gitlab.io/2017/10/compiling-kernel-module-only-w/o-whole-kernel-compilation/>
* <https://stackoverflow.com/questions/33384340/>

#### Arch Linux Docs

I then see <https://wiki.archlinux.org/title/Compile_kernel_module>, which
looks more correct. Recall that `uname -r` gives `5.10.0-14-amd64`

```sh
tar xf /usr/src/linux-source-5.10.tar.xz
cd linux-source-5.10/
cp /boot/config-$(uname -r) .config
make oldconfig
make nconfig
	# nothing to do
make SUBLEVEL=0 EXTRAVERSION=-14-amd64 modules_prepare
make M=arch/x86/kvm
ls arch/x86/kvm/kvm.ko
```

To install, use

```
sudo rmmod kvm_intel
sudo rmmod kvm
sudo insmod arch/x86/kvm/kvm.ko
```

When install fails, maybe check version in `strings`
```
$ sudo insmod arch/x86/kvm/kvm.ko
insmod: ERROR: could not insert module arch/x86/kvm/kvm.ko: Invalid module format
$ strings /usr/lib/modules/5.10.0-14-amd64/kernel/arch/x86/kvm/kvm.ko | grep 5.10.
5.10.0-14-amd64
vermagic=5.10.0-14-amd64 SMP mod_unload modversions 
$ strings arch/x86/kvm/kvm.ko | grep 5.10.
5.10.0-14-amd64
vermagic=5.10.113-14-amd64 SMP mod_unload modversions 
$ 
```

However, even fixing the version string, install still fails.

Asking on <https://stackoverflow.com/questions/72419524/>
`->` <https://superuser.com/questions/1723629/>

#### Compile first

To make things easier, we compile another version of 5.17.9, but compile KVM as
a module. Hopefully after that the `kvm_main.o` can be found easily again.

Following previous process, compiled 5.17.9 again, but with KVM as a module.

Install using

```
sudo apt-get install ./linux-image-5.17.9_5.17.9-1_amd64.deb
```

There are 3 files named `kvm.ko` generated in build directory
```
24570c3d66d1e1f261887042d07ef169  ./arch/x86/kvm/kvm.ko
94104e41c35a12e1690388ec9e4783f0  ./debian/linux-image/lib/modules/5.17.9/kernel/arch/x86/kvm/kvm.ko
1f3c5f9c02494e9085f9970317ffe0a5  ./debian/linux-image-dbg/usr/lib/debug/lib/modules/5.17.9/kernel/arch/x86/kvm/kvm.ko
94104e41c35a12e1690388ec9e4783f0  /usr/lib/modules/5.17.9/kernel/arch/x86/kvm/kvm.ko
```

Now, `./arch/x86/kvm/kvm.ko` can be loaded correctly using `insmod`

We can also follow the Arch Linux tutorial to build KVM quickly

```sh
cp ../../linux-5.17.9/.config .
make oldconfig
# The following takes < 5s
time make modules_prepare -j `nproc`
# The following takes 12s
time make M=arch/x86/kvm -j `nproc`
ls arch/x86/kvm/kvm.ko

sudo rmmod kvm_intel
sudo rmmod kvm
sudo insmod arch/x86/kvm/kvm.ko
sudo insmod arch/x86/kvm/kvm-intel.ko
lsmod | grep kvm
```

For some reason, the cursor and mouse does not work in new Kernel. The keyboard
and Internet works.

### Modifying KVM source code

Create git directory and create sh files

```
$ cat build.sh 
#!/bin/bash
set -xe
make distclean
cp ../../linux-5.17.9/.config .
make oldconfig
make modules_prepare -j `nproc`
make M=arch/x86/kvm -j `nproc`
ls arch/x86/kvm/kvm.ko
$ cat install.sh 
#!/bin/bash
set -xe
sudo rmmod kvm_intel
sudo rmmod kvm
sudo insmod arch/x86/kvm/kvm.ko
sudo insmod arch/x86/kvm/kvm-intel.ko
lsmod | grep kvm
$ 
```

Add print debugging using something like

```
	printk(KERN_ERR
			"LXY: %s:%d: %s(): hello world", __FILE__, __LINE__, __func__);
```

Can see result in `dmesg`

```
$ sudo dmesg -w | grep LXY
[  979.615938] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616147] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616152] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616153] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616629] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616631] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616632] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.616663] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.621342] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
[  979.621346] LXY: arch/x86/kvm/lapic.c:2788: kvm_x2apic_msr_write(): hello world
^C
$ 
```

When developing, only `make M=arch/x86/kvm -j $(nproc)` in `build.sh` need to
be executed. Then `install.sh` should be executed.

Looks like `kvm_lapic_reg_write()` is not called with `msr=APIC_EOI`.

Looks like the actual call stack is

```
VMEXIT due to Virtualized EOI (45 = EXIT_REASON_EOI_INDUCED)
	handle_apic_eoi_induced
		kvm_apic_set_eoi_accelerated
			trace_kvm_eoi (not interesting)
			kvm_ioapic_send_eoi
			kvm_make_request (likely not interesting)
```

Note that sometimes the last line of output may be lost. To prevent this, for
each printk can replace it with printing 2 lines intead.

After some debugging, become confused.

```
apic->isr_count
apic->highest_isr_cache
find_highest_vector(apic->regs + APIC_ISR)
```

In xmhf git `37643df66`, Linux patch `b.diff`, the output is

```
[ 4172.548263] LXY: EOI 0 Bitmap 0x1
[ 4172.548264] LXY: EOI 1 Bitmap 0x0
[ 4172.548264] LXY: EOI 2 Bitmap 0x0
[ 4172.548265] LXY: EOI 3 Bitmap 0x0
[ 4172.548265] LXY: EOI   Bitmap end
[ 4172.550165] LXY: EOI 0 Bitmap 0x1000000000001
[ 4172.550166] LXY: EOI 1 Bitmap 0x0
[ 4172.550167] LXY: EOI 2 Bitmap 0x0
[ 4172.550168] LXY: EOI 3 Bitmap 0x0
[ 4172.550168] LXY: EOI   Bitmap end
[ 4172.550452] LXY: <EOI ACCEL arch/x86/kvm/lapic.c:1283: kvm_apic_set_eoi_accelerated() 1 -1 48
[ 4172.550453] LXY: <EOI ACCEL arch/x86/kvm/lapic.c:1284: kvm_apic_set_eoi_accelerated() 1 -1 48
[ 4172.550454] LXY:   <IOAPIC EOI ACCEL arch/x86/kvm/lapic.c:1221: kvm_ioapic_send_eoi()   1 -1 48
[ 4172.550456] LXY:   <IOAPIC EOI ACCEL arch/x86/kvm/lapic.c:1222: kvm_ioapic_send_eoi()   1 -1 48
[ 4172.550457] LXY:   IOAPIC EOI ACCEL 3> arch/x86/kvm/lapic.c:1247: kvm_ioapic_send_eoi() 1 -1 48
[ 4172.550458] LXY:   IOAPIC EOI ACCEL 3> arch/x86/kvm/lapic.c:1248: kvm_ioapic_send_eoi() 1 -1 48
[ 4172.550459] LXY: EOI ACCEL> arch/x86/kvm/lapic.c:1291: kvm_apic_set_eoi_accelerated() 1 -1 48
[ 4172.550459] LXY: EOI ACCEL> arch/x86/kvm/lapic.c:1292: kvm_apic_set_eoi_accelerated() 1 -1 48
[ 4172.550462] LXY: <EOI ACCEL arch/x86/kvm/lapic.c:1283: kvm_apic_set_eoi_accelerated() 1 -1 -1
[ 4172.550463] LXY: <EOI ACCEL arch/x86/kvm/lapic.c:1284: kvm_apic_set_eoi_accelerated() 1 -1 -1
[ 4172.550463] LXY:   <IOAPIC EOI ACCEL arch/x86/kvm/lapic.c:1221: kvm_ioapic_send_eoi()   1 -1 -1
[ 4172.550464] LXY:   <IOAPIC EOI ACCEL arch/x86/kvm/lapic.c:1222: kvm_ioapic_send_eoi()   1 -1 -1
[ 4172.550465] LXY:   IOAPIC EOI ACCEL 3> arch/x86/kvm/lapic.c:1247: kvm_ioapic_send_eoi() 1 -1 -1
[ 4172.550466] LXY:   IOAPIC EOI ACCEL 3> arch/x86/kvm/lapic.c:1248: kvm_ioapic_send_eoi() 1 -1 -1
[ 4172.550466] LXY: EOI ACCEL> arch/x86/kvm/lapic.c:1291: kvm_apic_set_eoi_accelerated() 1 -1 -1
```

Looks like the hypervisor is not responsible for clearing ISR. The hardware is.
This can be confirmed in Intel i3 "28.1.4 EOI Virtualization"
> `VISR[Vector] := 0; (see Section 28.1.1 for definition of VISR)`

We can easily confirm that when XMHF has EFLAGS.IF = 0. So we assume that maybe
the hardware is wrong.

Blockers
* Not sure what is the expected behavior for EOI virtualization
* Not sure whether it is a bug in Intel's CPU / KVM / XMHF
* Cannot debug with nested KVM + GDB

Future
* If can reproduce this problem in LHV, may debug easier. Currently not
  familiar with Linux's interrupts
* If the `!env->exception_has_payload` problem can be fixed, may debug things
  using GDB
* If can debug physical CPU using Intel's tools, things may become better

### Disable EOI virtualization

From debugging above we suspect that the EOI virtualization path may be wrong
(either KVM or hardware's bug). So we try to disable this code path.

Accoding to Intel's manual, looks like EOI virtualization is binded to APIC
virtualization. So we can modify KVM code to let it think that the CPU does not
support x2APIC virtualization. The detection is done in
`cpu_has_vmx_virtualize_x2apic_mode()`.

Looks like after modifying `cpu_has_vmx_virtualize_x2apic_mode()` to always
return 0, the problem is solved. The guest still uses x2APIC (XMHF output
contains x2APIC, and Linux's lscpu shows the x2apic flag). So we are now very
confident that this bug is not in XMHF.

We should submit bug to KVM. We think that it is hard to reproduce this bug on
LHV, because it has something to do with IO APIC.

Submitted bug <https://bugzilla.kernel.org/show_bug.cgi?id=216045>. KVM bugs
will be tracked in `bug_076`.

## Fix

`a8610d2f9..78b56c9ca`
* Add documentation for CI
* Support Jenkins CI in Debian 11
* Remove x2APIC from CI

