# Write light-weight hypervisor

## Scope
* All XMHF
* Git `ee71c1210`

## Behavior
To implement nested virtualization, we need an existing hypervisor that we know
well. Implementing one based on current XMHF structure seems the best choice.

## Debugging

We start a new branch called `lhv`. This branch is based on `xmhf64` commit
`ee71c1210`.

### Optimize

First we should modify XMHF to make it compile and run fast.

For running, looks like before entering VM operations, the most time-consuming
operation happens when serial shows the following (see `**HERE**`)

```
    INIT(early): *UNTRUSTED* gold runtime: 4d 34 a2 ed 34 52 0e 0e 93 43 15 fa 42 85 96 35 
    INIT(early): *UNTRUSTED* gold runtime: af b8 bc db 
hashandprint: processing 0x0e0e1000 bytes at addr 0x10200000

**HERE**

    INIT(early): *UNTRUSTED* comp runtime: : 72 55 a1 c7 43 19 95 b4 5d 97 c2 bc 75 d5 4f dc 5a af d7 8d

    INIT(early): *UNTRUSTED* gold SL low 64K: 47 5b 30 0d fb c1 51 76 ad bc 40 e9 d4 90 41 b7 
    INIT(early): *UNTRUSTED* gold SL low 64K: 5f 1d f5 de 
hashandprint: processing 0x00010000 bytes at addr 0x10000000
    INIT(early): *UNTRUSTED* comp SL low 64K: : 47 5b 30 0d fb c1 51 76 ad bc 40 e9 d4 90 41 b7 5f 1d f5 de

    INIT(early): *UNTRUSTED* gold SL above 64K: 5c 7b 01 a7 8c 62 4a f1 60 95 7d 70 be 12 a5 5e 
    INIT(early): *UNTRUSTED* gold SL above 64K: d4 83 a3 b8 
hashandprint: processing 0x001f0000 bytes at addr 0x10010000
```

So looks like the time is wasted when compiling the runtime. We can simply ask
XMHF not to hash things. This is done in git `a2d7c7abe`.

Copying the runtime to 256M address is also time consuming. This is not
avoidable. So we need to decrease runtime size.

Other things run relatively fast.

When building, should use the simplest hypapp:
```sh
./build.sh amd64 --app hypapps/helloworld --no-ucode
```

After implementing `2388cc8e8`, looks like we can remove tomcrypt and tommath
libraries. This can save compile time.

In git `7d589f49c`, prevent building not necessary outside libraries.

We now investigate the object files that has large bss sections. This is
indirectly reflected by searching for `fallocate` in `549eb6838`. The results
are:
```sh
fallocate -d ./arch/x86/vmx/bplt-x86vmx-data.o
fallocate -d ./arch/x86/svm/bplt-x86svm-data.o
fallocate -d ./arch/x86/vmx/dmap-vmx-data.o
fallocate -d arch/x86/svm/memp-x86svm-data.o
fallocate -d arch/x86/vmx/memp-x86vmx-data.o
fallocate -d xmhf-mm.o
fallocate -d rntm-data.o
```

We can also view memory usage manually. The list of files come from the command
during `make`:
```sh
for i in ./xmhf-mm/xmhf-tlsf.o  ./xmhf-mm/xmhf-mm.o ... ./xmhf-startup/arch/x86/rntm-x86-data.o; do echo $i; readelf -SW $i | grep bss; done
```

Looks like the largest file is `./xmhf-mm/xmhf-mm.o`. We can remove it and some
other unused STL modules. Now things become very fast.

In git `666dce7fe`, basically can run lhv very quickly (less than 5 seconds).
Build is also very quick (less than 10 seconds).

Then we write `gr32` and `gr64` in `alias.sh` to remove GRUB timeout and give
it only one choice. Now GRUB is very fast. The configurations are
`bug_070/grub-amd64.cfg` and `bug_070/grub-i386.cfg`.

### Double fault when enable interrupts

Then we add console and timer drivers. However, we have trouble enabling
interrupts. Git is `2a8c90379`. Sending software interrupts using the INT
instruction works well, but setting EFLAGS.IF will result in double fault.

#### TSS

Looks like the problem is TSS. In QEMU, can use `info registers` to print
current registers. Pebbles kernel shows:
`TR =0008 00100950 00000067 00008b00 DPL=0 TSS32-busy`, but LHV shows
 TR =0018 10209340 00000067 00008b00 DPL=0 TSS32-busy
`TR =0000 0000000000000000 0000ffff 00008b00 DPL=0 TSS64-busy`.

Found a bug in 32-bit xmhf:
`t= (TSSENTRY *)((u32)gdt_base + __TRSEL );` should be something like
`t= (TSSENTRY *)((u32)hva2sla(gdt_base) + __TRSEL );`

This program can help printing the values.
```
printf("\nt  = 0x%016llx", (long long)(uintptr_t)(void *)t);
printf("\ntb = 0x%016llx", (long long)(uintptr_t) tss_base);
printf("\n*t = 0x%016llx", (long long)*(long long *)t);
HALT();
```

Bug fixed in `d2bd1d05a`.

Then added TSS, git `020c5cf3b`. However, the error is the same.

Also noticed that even 32-bit XMHF uses PAE paging for the host. So it should
be possible to support PAE guests easily (even these guests access PA > 4 GiB).
Just need to modify `xmhf_sl_arch_x86_setup_runtime_paging()`, change
`ADDR_4GB` to `MAX_PHYS_ADDR`.

Now we think that the problem is probably not related to TSS.

#### Debugging QEMU

When running without KVM and with `-d int`, can see the following log

```
     0: v=08 e=0000 i=0 cpl=0 IP=0008:102036fa pc=102036fa SP=0010:10b56f6c env->regs[R_EAX]=00000006
EAX=00000006 EBX=102092e0 ECX=0000002e EDX=00000040
ESI=10206da0 EDI=1020a99c EBP=10b56fa4 ESP=10b56f6c
EIP=102036fa EFL=00000206 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0018 10209340 00000067 00008900 DPL=0 TSS32-avl
GDT=     1020a980 0000001f
IDT=     10206010 000003ff
CR0=80000015 CR2=00000000 CR3=10b4e000 CR4=00000030
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000206 CCO=EFLAGS  
EFER=0000000000000000
```

Note: sample command:

```
qemu-system-i386 -m 512M \
	--drive media=disk,file=.../c.img,index=2 -cpu Haswell \
	-gdb tcp::1234 -serial stdio -smp 1 -d int
```

This log is printed by `do_interrupt_all()` in QEMU. We can probably debug
this. Also, the double fault / triple fault is generated in
`check_exception()` in `excp_helper.c`:

```c
    if ((first_contributory && second_contributory)
        || (env->old_exception == EXCP0E_PAGE &&
            (second_contributory || (intno == EXCP0E_PAGE)))) {
        intno = EXCP08_DBLE;
        *error_code = 0;
    }
```

The stack trace is
```
#0  do_interrupt_all (cpu=cpu@entry=0x5555567b1080, intno=8, is_int=is_int@entry=0, error_code=error_code@entry=0, next_eip=next_eip@entry=0, is_hw=is_hw@entry=1) at ../target/i386/tcg/seg_helper.c:1042
#1  0x0000555555ad6374 in do_interrupt_x86_hardirq (is_hw=1, intno=<optimized out>, env=0x5555567b98d0) at ../target/i386/tcg/seg_helper.c:1112
#2  x86_cpu_exec_interrupt (cs=0x5555567b1080, interrupt_request=<optimized out>) at ../target/i386/tcg/seg_helper.c:1165
#3  0x0000555555bded56 in cpu_handle_interrupt (last_tb=<synthetic pointer>, cpu=0x5555567b1080) at ../accel/tcg/cpu-exec.c:763
#4  cpu_exec (cpu=cpu@entry=0x5555567b1080) at ../accel/tcg/cpu-exec.c:917
#5  0x00007fffdf6d2ad7 in tcg_cpus_exec (cpu=cpu@entry=0x5555567b1080) at ../accel/tcg/tcg-accel-ops.c:67
#6  0x00007fffdf6d2bf7 in mttcg_cpu_thread_fn (arg=0x5555567b1080) at ../accel/tcg/tcg-accel-ops-mttcg.c:70
#7  0x0000555555d22373 in qemu_thread_start (args=0x5555565d9c90) at ../util/qemu-thread-posix.c:541
#8  0x00007ffff6cc085a in start_thread (arg=<optimized out>) at pthread_create.c:443
#9  0x00007ffff6c604c0 in clone3 () at ../sysdeps/unix/sysv/linux/x86_64/clone3.S:81
```

Looks like the interrupt happens to use the double fault's vector. There are
some configurations needed on the PIC. See Pebbles `410kern/x86/pic.c` as an
example.

This also explains why the double fault we receive looks like that it does not
have an error code.

Git `2c2028e3d` fixes the problem. Now everything looks good.

Previous ideas
* Try to enable interrupts earlier (e.g. in sl)
	* This will not work
* Compare with things that are known to work
	* This is what we did, but we compared code
* Write standalone code that can setup IDT
	* This will be complicated
* Try on real hardware
	* This will not work
* Possible to disable KVM and debug QEMU source code
	* We did this, and figured out that we encountered interrupt, not double
	  fault.

### Fast install

For lhv we do not depend on Debian, so can install quickly using `test3.py`

```sh
python3 ../xmhf64/.jenkins/test3.py \
	--skip-reset-qemu --skip-test --boot-dir ../xmhf64/.jenkins/boot/ \
	--subarch i386 --xmhf-bin . --work-dir /tmp/xmhf-grub-i386
```

### LAPIC timer

15410's `p3/410kern/smp/smp4.pdf` is very helpful. APIC is documented in Intel
v3 "10.5.4 APIC Timer".

In git `968ebcefa`, completed set up of LAPIC timer. A problem is that LAPIC
timer is not as accurate as PIT, but we can live with it.

