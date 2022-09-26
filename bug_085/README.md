# Running XMHF in XMHF

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `eecb1ffce`
* Git branch `lhv` commit `6a8866c1e`

## Behavior
After `bug_084`, I think it should be possible to run XMHF in XMHF. So try it.

## Debugging

### Improving grub.py

`grub.py` needs to be updated for XMHF in XMHF to work. For example, currently
XMHF only boots correctly when it is the first disk. We also add some features,
like setting timeout (to debug GRUB).

In `xmhf64 3b8f57996..307d583ed`, rewrite most part of `grub.py` to implement
the above.

In previous work (undocumented?), `lhv-dev 803252ae3..a31c469d7` contains
`control.py` that automatically tests GRUB image with change to list of modules
copied. This was used to create a minimal GRUB image. We use a similar approach
to figure out the minimal GRUB image for booting Windows.

The result I get is
```
affs.mod
boot.mod
bufio.mod
chain.mod
command.lst
crypto.mod
disk.mod
drivemap.mod
extcmd.mod
gcry_tiger.mod
gettext.mod
gfxterm_background.mod
mmap.mod
msdospart.mod
nilfs2.mod
normal.mod
ntfs.mod
parttool.lst
parttool.mod
relocator.mod
terminal.mod
test.mod
tga.mod
verifiers.mod
video.mod
```

It looks difficult to make the GRUB image use the FAT file system. When I try
it, I get the "unknown filesystem" error from GRUB. So give up.

### Loading two XMHFs

Change `__TARGET_BASE_SL` to `0x20000000` for one XMHF to solve the address
collision problem. Looks like XMHF is taking much more memory than I expected.
So placing it below 256M (like LHV) is not realistic. We also need to increase
QEMU memory. Sample QEMU command is

```sh
./bios-qemu.sh -smp 1 -m 1G --gdb 2198 --qb32 -d build32 +1 -d build32lhv +1 -d debian11x86 +1
```

In `xmhf64 efdafbd78`, `xmhf64-nest 59bf041f0`, `lhv 0d49b5ffc`, can set
`__TARGET_BASE_SL` using `./configure` or `./build.sh`

The first problem we see at `xmhf64-nest 88863b9ef` and `xmhf64 3f0c4c695` is:

```
Fatal: Halting! Condition 'cache01 == HPT_PMT_UC || cache01 == HPT_PMT_WB' failed, line 467, file arch/x86/vmx/nested-x86vmx-ept12.c
```

This problem happens in the outer XMHF. This is simply an unimplemented feature
in EPT.

EPT entries have 5 possible values, as defined in Intel v3
"27.2.7.2 Memory Type Used for Translated Guest-Physical Addresses" (page 1023):
`0 = UC; 1 = WC; 4 = WT; 5 = WP; and 6 = WB.`. This is similar to MTRRs. We use
MTRR's rules to handle merging, see Intel v3 "11.11.4.1 MTRR Precedences"
(page 454).

Another table is "11.5.2.2 Selecting Memory Types for Pentium III and More
Recent Processor Families" (page 441). But I think MTRR's rules are better.

The EPT problem is solved in `xmhf64-nest daba03cff`.

### Triple fault

The new problem is that L2 guest receives triple fault. The serial port shows

```
...
CPU(0x00): EPT: 0x00110000 0x00110000 0x00110000
CPU(0x00): EPT: 0x00111000 0x00111000 0x00111000
CPU(0x00): EPT: 0x00112000 0x00112000 0x00112000
CPU(0x00): EPT: 0xffe5b625 0xffe5b000 0xffe5b000
CPU(0x00): nested vmexit 2
CPU(0x00): Unhandled intercept: 2 (0x00000002)
	CPU(0x00): EFLAGS=0x00010097
	SS:ESP =0x0010:0x0007fc24
	CS:EIP =0x0008:0x00008d03
	IDTR base:limit=0x00000000:0x0000
	GDTR base:limit=0x00008280:0x0027
```

The L2 guest is in protected mode, without paging. `0x8d03` is `0xff`, which
looks like invalid opcode for x86. Also, when setting a break point at `0x8d03`
using GDB, only the L2 guest hits the break point. So I suspect that this is
some code in error handling. We may need to start debugging GRUB.

In `xmhf64-nest-dev 816dbd06d`, print EIP (or CS:EIP for real mode) when EPT
violation. Can see that there are not a lot of intercepts into L1.

```
CPU(0x00): EPT: 0x00000000 CS:EIP=0x00007c00
CPU(0x00): nested vmentry
CPU(0x00): EPT: 0x0000fffe CS:EIP=0x00007c00
CPU(0x00): EPT: 0x000ffea5 CS:EIP=0x000ffea5
CPU(0x00): EPT: 0x000fd2c6 CS:EIP=0x000fd2c6
CPU(0x00): EPT: 0x000e8f10 CS:EIP=0x000fd2d3
CPU(0x00): EPT: 0x000e970c CS:EIP=0x000fd2db
CPU(0x00): EPT: 0x000f5ff0 CS:EIP=0x000ff15c
CPU(0x00): EPT: 0x000f7246 CS:EIP=0x000f7246
CPU(0x00): EPT: 0x000f8194 CS:EIP=0x000f8194
CPU(0x00): EPT: 0x000f6d65 CS:EIP=0x000f6d65
CPU(0x00): EPT: 0x000fcfc8 CS:EIP=0x000fcfc8
CPU(0x00): EPT: 0x000fe829 CS:EIP=0x000fe829
CPU(0x00): EPT: 0x00007c00 CS:EIP=0x00007c00
CPU(0x00): EPT: 0x00001ffe CS:EIP=0x00007c8c
CPU(0x00): EPT: 0x000c5663 CS:EIP=0x000c5663
CPU(0x00): EPT: 0x000c98b8 CS:EIP=0x000c5676
CPU(0x00): EPT: 0x000c487c CS:EIP=0x000c487c
CPU(0x00): EPT: 0x000c2402 CS:EIP=0x000c2402
CPU(0x00): EPT: 0x000c0dde CS:EIP=0x000c0dde
CPU(0x00): EPT: 0x000c1d32 CS:EIP=0x000c1d32
CPU(0x00): EPT: 0x000c67a6 CS:EIP=0x000c1d6d
CPU(0x00): EPT: 0x000b8280 CS:EIP=0x000c1dc5
CPU(0x00): EPT: 0x000fb83b CS:EIP=0x000fb83b
CPU(0x00): EPT: 0x000faaa1 CS:EIP=0x000faaa1
CPU(0x00): EPT: 0x00070000 CS:EIP=0x000fa719
CPU(0x00): EPT: 0x00008000 CS:EIP=0x00007d63
CPU(0x00): EPT: 0x0000ec05 CS:EIP=0x00008000
CPU(0x00): EPT: 0x0007fc60    EIP=0x00008313
CPU(0x00): EPT: 0x00108000    EIP=0x0000893f
CPU(0x00): EPT: 0x00014fff    EIP=0x000085b5
CPU(0x00): EPT: 0x00013fff    EIP=0x000085b5
CPU(0x00): EPT: 0x00100200    EIP=0x000085d2
CPU(0x00): EPT: 0x0000915d    EIP=0x00008720
CPU(0x00): EPT: 0x0000a15d    EIP=0x00008720
CPU(0x00): EPT: 0x0000b15d    EIP=0x00008720
CPU(0x00): EPT: 0x0000c15d    EIP=0x00008720
CPU(0x00): EPT: 0x0000d15d    EIP=0x00008720
CPU(0x00): EPT: 0x000101a1    EIP=0x0000874a
CPU(0x00): EPT: 0x000111a1    EIP=0x0000874a
CPU(0x00): EPT: 0x000121a1    EIP=0x0000874a
CPU(0x00): EPT: 0x00101000    EIP=0x0000876f
CPU(0x00): EPT: 0x0010b178    EIP=0x00008ac7
CPU(0x00): EPT: 0x0010c000    EIP=0x00008ac7
CPU(0x00): EPT: 0x0010d000    EIP=0x00008ac7
CPU(0x00): EPT: 0x0010e000    EIP=0x00008ac7
CPU(0x00): EPT: 0x0010f000    EIP=0x00008ac7
CPU(0x00): EPT: 0x00110000    EIP=0x00008ac7
CPU(0x00): EPT: 0x00111000    EIP=0x00008ac7
CPU(0x00): EPT: 0x00112000    EIP=0x00008ac7
CPU(0x00): EPT: 0xffe5b625    EIP=0x00008bfa
CPU(0x00): nested vmexit 2
CPU(0x00): Unhandled intercept: 2 (0x00000002)
	CPU(0x00): EFLAGS=0x00010097
	SS:ESP =0x0010:0x0007fc24
	CS:EIP =0x0008:0x00008d03
	IDTR base:limit=0x00000000:0x0000
	GDTR base:limit=0x00008280:0x0027
```

Next steps
* Why is there a EPT before "nested vmentry"
* Try to remove all EPTs in XMHF and run "KVM -> XMHF -> GRUB"
* Dump VMCS in different places and compare

Answer to "Why is there a EPT before 'nested vmentry'": this is caused by the
PDPTE CR3 bug in KVM. However it does not affect too much.

First we dump VMCS and compare. In `xmhf64-nest-dev c890250be`, serial
`20220709160410`, dump VMCS in L0 during VMLAUNCH at L1 and L2.

```sh
F=20220709160410
diff <(grep :L1: $F | cut -d : -f 2,4) <(grep :L2: $F | cut -d : -f 2,4)
```

Can see that the different VMCS fields are IO bitmap address, MSR load / store
address, and EPT pointer. I guess the content of the first two should be almost
exactly the same. It is more likely that the problem happens due to EPT.

In `xmhf64-dev f6bddc269`, print all VMEXITs. When running with only 1 level of
XMHF, can see that the first VMEXIT happens at EIP `0x00009321`.

```
CPU(0x00): VMCLEAR success.
CPU(0x00): VMPTRLD success.
CPU(0x00): VMWRITEs success.
VMEXIT 10 CR0=0x00000031 RIP=0x00009321
VMEXIT 18 CR0=0x00000030 RIP=0x000000ac
CPU(0x00): INT 15(e820): EDX=0x0000e820, EBX=0x534d4150, ECX=0x00000000, ES=0x0014, DI=0x6800
VMEXIT 18 CR0=0x00000030 RIP=0x000000ac
CPU(0x00): INT 15(e820): EDX=0x0000e820, EBX=0x534d4150, ECX=0x00000001, ES=0x0014, DI=0x6800
VMEXIT 18 CR0=0x00000030 RIP=0x000000ac
CPU(0x00): INT 15(e820): EDX=0x0000e820, EBX=0x534d4150, ECX=0x00000002, ES=0x0014, DI=0x6800
```

Looks like the problem happens before the first `L2 -> L1` VMEXIT. So we
continue by manipulating EPT in 1 level XMHF.

In `xmhf64-dev 50cf5be85`, serial `20220709170715`, implement EPT from zero.
This time see a different error. After EPT violations in `RIP=0x00008ac7`,
there are 2 more errors. Then a lot of EPT errors happen at `RIP=0x00008bfa`
with a wild number of pages. This may be a different bug than the nested
virtualization one, but is also unacceptible.

We try running things on Bochs, and things get interesting. Before, we are
running `xmhf64-dev 50cf5be85` and then `debian11x86`. The results are above.
Now in Bochs, we run `xmhf64-dev 50cf5be85` and `xmhf64-nest-dev` (image
built by `grub.py`). The result is `20220709171933`, which is basically
successful. However, in QEMU when running the same thing, I get
`20220709172712`, which is triple fault, but a new behavior.

For consistency, we will try to use images built by `grub.py` for now. Another
example is `grub_windows.img`.

Even though there are small differences in different configurations (e.g.
detailed list of pages EPT violated, RIP that triggers triple fault), I guess
the root cause may be common. So we debug the fastest one. The good thing is
that the same configuration usually has consistent behavior.

Note: to workaround this problem, can pre-fill EPT with ID-map entries in L0
hypervisor.

For `xmhf64-dev` and then `xmhf64-nest-dev` in QEMU, I noticed that RSP is
wrong. In `xmhf64-dev f4d59ca6e`, after printing RSP the output looks like the
following:

```
EPT: 0x00128000 RIP=0x00008ac7 RSP=0x0007fc2c
EPT: 0x00129000 RIP=0x00008ac7 RSP=0x0007fc2c
EPT: 0x0012a000 RIP=0x00008ac7 RSP=0x0007fc2c
EPT: 0x0012b000 RIP=0x00008ac7 RSP=0x0007fc2c
EPT: 0x0012c000 RIP=0x00008ac7 RSP=0x0007fc2c
EPT: 0x52d2bef3 RIP=0x00008a39 RSP=0x0007fc1c
EPT: 0x47ca0424 RIP=0x00008a4f RSP=0x0007fc14
EPT: 0xfffffffc RIP=0x00008a42 RSP=0x00000000
EPT: 0xc014af48 RIP=0x000089f2 RSP=0xfffffff4
VMEXIT 2 CR0=0x00000031 RIP=0x00008a11
CPU(0x00): Unhandled intercept: 2 (0x00000002)
	CPU(0x00): EFLAGS=0x00010047
	SS:ESP =0x0010:0xfffffff4
	CS:EIP =0x0008:0x00008a11
	IDTR base:limit=0x00000000:0x0000
	GDTR base:limit=0x00008280:0x0027
```

In `RIP=0x00008a42`, RSP is wrong because it should point to NULL. Actually,
`RIP=0x00008a39` is also strange because it accesses an unreasonable address.

We can break at `0x00008a39` using a GDB script:

```
hb *xmhf_partition_arch_x86vmx_initializemonitor
c
d
hb *0x00008a39
c
```

The strange thing happens because at `0x00008a39` the instructions look like

```
(gdb) p/x $esi
$2 = 0x8d2a
(gdb) x/4i $eip
=> 0x8a39:	and    %esi,0x52d231c9(%esi)
   0x8a3f:	inc    %edx
   0x8a40:	push   %edx
   0x8a41:	push   %eax
(gdb) 
```

It is very strange to add 0x52d231c9 to ESI. We need to disassemble to get
a better idea.

At the break point, use GDB's `dump binary memory /tmp/a.bin 0x0 0x100000` to
dump the memory to file. Then compare with `./tools/ci/boot/a.img`. Can see
that memory's `0x7c00 - 0x7e00` is similar to `a.img`'s `0x0 - 0x200`. Memory's
`0x8200 - 0x9000` is similar to `a.img`'s `0x400 - 0x1200`.

```
dd if=./tools/ci/boot/a.img bs=512 skip=2 count=7 > /tmp/b.bin
objdump --adjust-vma=0x8200 -b binary -m i8086 -D /tmp/b.bin > a.s.16
objdump --adjust-vma=0x8200 -b binary -m i386 -D /tmp/b.bin > a.s.32
```

`a.s.32` is attached in this bug. We can see that the instruction at `0x8a39`
completely changed. The expected instructions are:

```
    8a39:	0f b6 c9             	movzbl %cl,%ecx
    8a3c:	31 d2                	xor    %edx,%edx
    8a3e:	52                   	push   %edx
```

The change is caused by `0x8a39` byte change from 0x0f to 0x21. However, it is
very strange to see this change. We can use hexdump to compare the binaries

```diff
2c2
< 00008210  5f 68 00 00 5d 07 00 00  ff ff ff 00 fa 31 c0 8e  |_h..]........1..|
---
> 00008210  5f 68 00 00 5d 07 00 00  ff ff ff 81 fa 31 c0 8e  |_h..]........1..|
7c7
< 00008260  56 07 00 00 f0 ff 07 00  eb 16 8d b4 26 00 00 00  |V...........&...|
---
> 00008260  56 07 00 00 60 fc 07 00  eb 16 8d b4 26 00 00 00  |V...`.......&...|
9,11c9,11
< 00008280  00 00 00 00 00 00 00 00  ff ff 00 00 00 9a cf 00  |................|
< 00008290  ff ff 00 00 00 92 cf 00  ff ff 00 00 00 9e 00 00  |................|
< 000082a0  ff ff 00 00 00 92 00 00  eb 16 8d b4 26 00 00 00  |............&...|
---
> 00008280  00 00 00 00 00 00 00 00  ff ff 00 00 00 9b cf 00  |................|
> 00008290  ff ff 00 00 00 93 cf 00  ff ff 00 00 00 9f 00 00  |................|
> 000082a0  ff ff 00 00 00 93 00 00  eb 16 8d b4 26 00 00 00  |............&...|
13c13
< 000082c0  27 00 80 82 00 00 00 04  00 00 00 00 00 00 00 00  |'...............|
---
> 000082c0  27 00 80 82 00 00 ff 03  00 00 00 00 00 00 00 00  |'...............|
132c132
< 00008a30  11 c1 ea 05 29 11 f9 eb  d8 0f b6 c9 31 d2 52 42  |....).......1.RB|
---
> 00008a30  11 c1 ea 05 29 11 f9 eb  d8 21 b6 c9 31 d2 52 42  |....)....!..1.RB|
#                                       ^^
134c134
< 00008a50  44 24 04 09 44 24 08 f9  11 d2 58 d1 24 24 e2 e1  |D$..D$....X.$$..|
---
> 00008a50  a0 24 04 09 44 24 08 f9  11 d2 58 d1 24 24 e2 e1  |.$..D$....X.$$..|
#           ^^
140c140
< 00008ab0  45 fc c3 55 89 e5 83 ec  24 57 fc 89 df b9 36 1f  |E..U....$W....6.|
---
> 00008ab0  45 fc c3 55 89 e5 83 ec  24 57 00 89 df b9 36 1f  |E..U....$W....6.|
#                                          ^^
147c147
< 00008b20  8b 55 f8 c1 ea 05 01 d0  ba 00 03 00 00 f7 e2 05  |.U..............|
---
> 00008b20  8b 55 f8 c1 ea 05 01 d0  ba 8f 03 00 00 f7 e2 05  |.U..............|
#                                       ^^
169c169
< 00008c80  ff ff 52 b8 03 00 00 00  39 c2 76 02 89 c2 b1 06  |..R.....9.v.....|
---
> 00008c80  ff ff 52 b8 03 00 00 00  39 c2 76 60 89 c2 b1 06  |..R.....9.v`....|
#                                             ^^
176c176
< 00008cf0  45 f0 e2 d8 b1 04 d3 e2  01 55 e8 b8 22 03 00 00  |E........U.."...|
---
> 00008cf0  45 f0 e2 d8 b1 04 d3 e2  20 55 e8 b8 22 03 00 00  |E....... U.."...|
#                                    ^^
194c194
< 00008e10  d7 0c 1d 1e d9 d8 01 ed  bb a2 c2 b1 6c 83 30 9b  |............l.0.|
---
> 00008e10  d7 0c 1d 1e d9 d8 01 ed  bb a2 c2 b1 d5 83 30 9b  |..............0.|
#                                                ^^
204c204
< 00008eb0  36 df fb 2d c1 78 8d 01  e3 4e 00 80 98 43 67 0b  |6..-.x...N...Cg.|
---
> 00008eb0  36 df fb 2d c1 6c 8d 01  e3 4e 00 80 98 43 67 0b  |6..-.l...N...Cg.|
#                          ^^
```

A lot of these changes look like valid instructions. They do not make sense.
We need to investigate when the instructions are changed.

### What happens to `0x8000`

When looking at `20220709172712`, a strange thing is that when EPT violation at
`RIP=0x8000`, `0x8000` is full of 0s. I suspect this is not the expected
behavior. So we break at this address using GDB on normal XMHF. And then we
found something interesting. The GDB script is

```
hb *xmhf_partition_arch_x86vmx_initializemonitor
c
d
hb *0x00008000
c
x/10i 0x8000
```

When using `xmhf64 efdafbd78`, see
```
=> 0x8000:	push   %edx
   0x8001:	push   %esi
   0x8002:	mov    $0x39e8811b,%esi
   0x8007:	add    %ebx,-0x41(%esi)
   0x800a:	hlt    
```

However, when using `xmhf64-dev d3ec3b399`, see

```
=> 0x8000:	add    %al,(%eax)
   0x8002:	add    %al,(%eax)
   0x8004:	add    %al,(%eax)
   0x8006:	add    %al,(%eax)
   0x8008:	add    %al,(%eax)
```

In `xmhf64-dev c293402c9`, serial `20220709212433`. Currently XMHF's logic is
* Set `*0x8000 = 0x5a5a5a5a5a5a5a5a`
* Start guest GRUB
* At `CS:EIP=0x00007d63`, see EPT violation at address `0x8000`. At this time
  still `*0x8000 = 0x5a5a5a5a5a5a5a5a`
* The next EPT violation is `CS:EIP=0x00008000`, at this point
  `*0x8000 = 0x0000000000000000`. However, it should be `0x0139e8811bbe5652`.

Now we need to debug the first 512 bytes of GRUB. First get an objdump

```
objdump --adjust-vma=0x7c00 -b binary -m i8086 -D ./tools/ci/boot/a.img | grep 7dfd -B 10000 > b.s.16
```

`bug_033` section `### Discovering states of GRUB entry` has information about
GRUB. Looks like we are on the right track.

Next, we break at `0x7d63`, which should be the `rep movsw %ds:(%si),%es:(%di)`
instruction that write to `0x8000`. We see that `DS:SI = 0x7000:0x0`,
`ES:DI = 0x0:0x8000`, `CX=0x100`.

In `xmhf64`, `0x70000` looks good (`push %edx; push %esi; ...`). In
`xmhf64-dev`, `0x70000` is full of 0s. Now we need to find out what happens
to `0x70000`.

### What happens to `0x70000`

`CS:EIP=0x000fa72e` is where `0x70000` is accessed. In `xmhf64-dev 31cb9621e`,
we can see that `0x000fa72e` is filled with 0s after being accessed.

`0x000fa72e` is `rep insl (%dx),%es:(%di)`, where `DX=0x1f0`,
`ES:DI=0x7000:0x0`, `CX=0x100`. From QEMU's `info mtree`, can see that `DX`
points to the IDE IO port. (Non-official) information can be found at
<https://wiki.osdev.org/ATA_PIO_Mode>.

Other IDE references
* <https://www.cs.cmu.edu/~410-f03/p4/p4disk.html>
* SeaBIOS source code: `git clone https://git.seabios.org/seabios.git`
* Reverse engineer the SeaBIOS in QEMU

It looks like the problem likely happens due to the `rep insl` instruction's
EPT violation. In `xmhf64-dev d6549ab30`, if we map `0x70000` from the
beginning, the serial output looks like

```
EPT: 0x000b8280 CS:EIP=0x000c1dc5 SS:ESP=0x000e938c     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x000fb850 CS:EIP=0x000fb850 SS:ESP=0x000e9c84     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x000f598c CS:EIP=0x000fb169 SS:ESP=0x000e9c60     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x000faab6 CS:EIP=0x000faab6 SS:ESP=0x000e9c5c     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x00008000 CS:EIP=0x00007d63 SS:ESP=0x00001fec     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x0139e8811bbe5652
EPT: 0x00071000 CS:EIP=0x000fa72e SS:ESP=0x000e9bf4     *0x8000=0x0139e8811bbe5652 *0x70000=0x0000000000821cea
EPT: 0x00072000 CS:EIP=0x000fa72e SS:ESP=0x000e9bf4     *0x8000=0x0139e8811bbe5652 *0x70000=0x0000000000821cea
```

However, if we disable it by changing `} else if (paddr == 0x70000) {` in
`memp-x86vmx.c`, the serial output becomes

```
EPT: 0x000faab6 CS:EIP=0x000faab6 SS:ESP=0x000e9c5c     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x00070000 CS:EIP=0x000fa72e SS:ESP=0x000e9bf4     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x5a5a5a5a5a5a5a5a
EPT: 0x00008000 CS:EIP=0x00007d63 SS:ESP=0x00001fec     *0x8000=0x5a5a5a5a5a5a5a5a *0x70000=0x0000000000000000
EPT: 0x0000ec05 CS:EIP=0x00008000 SS:ESP=0x00001ffe     *0x8000=0x0000000000000000 *0x70000=0x0000000000000000
EPT: 0x0007fc60    EIP=0x00008313    ESP=0x0007fc60     *0x8000=0x0000000000000000 *0x70000=0x0000000000000000
```

So pre-mapping `0x70000` should be a workaround to this bug. We should also
try to reproduce it by writing a small program using IDE.

### Calling BIOS

We first replace GRUB with code that we control to access BIOS. We use GRUB
source code at
<https://git.savannah.gnu.org/cgit/grub.git/tree/grub-core/boot/i386/pc/boot.S>
and reverse engineer GRUB running in QEMU and XMHF.

GRUB calls `int $0x13` at `0x7cd5`. GDB shows:
```
(gdb) x/10lx $si
0x7c05:	0x00010010	0x70000000	0x00000001	0x00000000
0x7c15:	0x00b90600	0xeaa4f302	0x00000621	0x3807bebe
0x7c25:	0x830b7504	0xfe8110c6
(gdb) info registers 
eax            0x4201              16897
edx            0x81                129
esp            0x1ffe              0x1ffe
ebp            0x0                 0x0
esi            0x7c05              31749
cs             0x0                 0
ds             0x0                 0
(gdb) 
```

The call should be: `AH = 0x42`, `DL = 0x81`, `*(DS:SI + 0) = 0x10`,
`*(DS:SI + 1) = 0x0`, `*(DS:SI + 2) = 0x0001`, `*(DS:SI + 4) = 0x0` (offset),
`*(DS:SI + 6) = 0x7000` (segment), `*(DS:SI + 8) = 0x00000001` (LBA low),
`*(DS:SI + 12) = 0x00000000` (LBA high).

In `xmhf64-dev 51f41b5b9`, wrote a simple real mode program to replace GRUB's
first sector. This program asks the BIOS to copy the disk to 0x8000, then
accesses 0x1234 to trigger an EPT so that 0x8000 is printed to the serial.
The result is:

```
...
EPT: 0x00007c00 CS:EIP=0x00007c00     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000b8000 CS:EIP=0x00007c12     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000fb850 CS:EIP=0x000fb850     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000f598c CS:EIP=0x000fb169     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000faab6 CS:EIP=0x000faab6     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x00008000 CS:EIP=0x000fa72e     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x00001234 CS:EIP=0x00007c59     *0x8000=0x0000000000000000
```

However, if we access 0x8000 beforehand (remove `#ifdef 0` in
`part-x86vmx-sup.S`) to trigger an EPT before BIOS, the behavior is correct:

```
...
EPT: 0x00007c00 CS:EIP=0x00007c00     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000b8000 CS:EIP=0x00007c12     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x00008800 CS:EIP=0x00007c2b     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000fb850 CS:EIP=0x000fb850     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000f598c CS:EIP=0x000fb169     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x000faab6 CS:EIP=0x000faab6     *0x8000=0x5a5a5a5a5a5a5a5a
EPT: 0x00001234 CS:EIP=0x00007c66     *0x8000=0x0139e8811bbe5652
```

This means that we are on the right direction. The next step is to remove BIOS
code and control IDE directly.

In `xmhf64-dev 17ee097c3`, simplify the code even more. VGA code are removed.
Now the logic is:
* Remove 0x8000 and 0x1000 from EPT
* Start guest
* EPT violation at 0x8000 due to `rep ins`. Change the next instruction to
  VMCALL
* VMCALL prints 0x8000.

The bad behavior is:

```
EPT:    0x00008000 CS:EIP=0x000fa719 *0x8000=0x5a5a5a5a5a5a5a5a (inst 67 f3 6d)
VMCALL: 0x00008000 CS:EIP=0x000fa71c *0x8000=0x0000000000000000
```

For the good behavior in QEMU, there will be no VMCALL:

```
EPT:    0x00008800 CS:EIP=0x00007c0b *0x8000=0x5a5a5a5a5a5a5a5a
EPT:    0x00001234 CS:EIP=0x00007c22 *0x8000=0x0139e8811bbe5652
```

In Bochs, for some reason modifying the instruction to VMCALL does not work.
So will see

```
EPT:    0x00008000 CS:EIP=0x000f2c49 *0x8000=0x5a5a5a5a5a5a5a5a (inst f3 66 6d)
EPT:    0x00001234 CS:EIP=0x00007c15 *0x8000=0x0139e8811bbe5652
```

Note on disassemble:

```
   0:	f3 66 6d             	rep insl (%dx),%es:(%di)
   3:	cc                   	int3   
   4:	67 f3 6d             	rep insw (%dx),%es:(%edi)
   7:	cc                   	int3   
```

### Minimizing disk read code

Looks like IDE is too complicated. May going to drop this step

### Debugging KVM

By reading code, I guess the related call stack should be:

```
vmx_handle_exit()
	nested_vmx_reflect_vmexit()
	handle_io()
		/* Notice that string = 1, which is special for this bug */
		kvm_emulate_instruction()
			x86_emulate_instruction()
				x86_emulate_insn() ?
					...
						em_in()
							pio_in_emulated()
```

Use `bug_084`'s notes to print debug KVM. `vmcs_readl(GUEST_RIP)` can be used
to read the VMCS.

This bug is reproducible on Debian Linux `5.18.9` (self-compiled).

First add the following to `__vmx_handle_exit()`. Then grep for `nest 1`.

```
	printk(KERN_ERR "LXY: %s:%d: %s(): exit reason% 3d, RIP 0x%08lx, nest %d",
			__FILE__, __LINE__, __func__, vmcs_read32(VM_EXIT_REASON),
			vmcs_readl(GUEST_CS_BASE) + vmcs_readl(GUEST_RIP),
			(int) is_guest_mode(vcpu));
```

We can see the following:

```
...
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa404, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa404, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa404, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa591, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa591, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 30, RIP 0x000fa591, nest 1
.../vmx.c:6083: __vmx_handle_exit(): exit reason 18, RIP 0x000fa594, nest 1
```

QEMU serial shows

```
EPT:    0x00008000 CS:EIP=0x000fa591 *0x8000=0x5a5a5a5a5a5a5a5a (inst 67 f3 6d)
VMCALL: 0x00008000 CS:EIP=0x000fa594 *0x8000=0x0000000000000000
```

`0xfa591` is the place where `rep insw` is called. Looks like only the last 4
`L2 -> L0` VMEXITs are interested. Actually after the first VMEXIT, `L1`
handles the EPT (because there are a lot of VMEXITs with `nest 0`).

When running the good version (i.e. access 0x8000 beforehand), the latter 2
VMEXITs on 0x000fa591 are the same. The first VMEXIT is gone.

Using `dump_stack()` in Linux can print a function's call stack. Updated call
stack:

```
...
	kvm_arch_vcpu_ioctl_run()
		vcpu_run()
			...
			vcpu_enter_guest() // kvm_x86_ops.handle_exit is vmx_handle_exit
				vmx_handle_exit() // RIP 0x000fa591, nest 1, reason IO
					handle_io()
						kvm_emulate_instruction()
							x86_emulate_instruction()
								x86_emulate_insn()
									?
										emulator_pio_in() ?
											__emulator_pio_in() ?
												emulator_pio_in_out()
													kernel_pio()
														kvm_io_bus_read()
															__kvm_io_bus_read()
																kvm_iodevice_read()
								vcpu->arch.complete_userspace_io = complete_emulated_pio
kvm_vcpu_ioctl() // ioctl=KVM_RUN
	kvm_arch_vcpu_ioctl_run()
		complete_emulated_pio() // vcpu->arch.complete_userspace_io
			complete_emulated_pio()
				complete_emulated_io()
					kvm_emulate_instruction()
						x86_emulate_instruction()
							x86_emulate_insn()
							// ctxt->have_exception
							inject_emulated_exception()
								kvm_inject_emulated_page_fault()
kvm_vcpu_ioctl() // ioctl=KVM_RUN
	// L1
kvm_vcpu_ioctl() // ioctl=KVM_RUN
	kvm_arch_vcpu_ioctl_run()
		vcpu_run()
			vcpu_enter_guest()
				vmx_handle_exit() // RIP 0x000fa591, nest 1, reason IO
					handle_io()
						kvm_emulate_instruction()
							x86_emulate_instruction()
								x86_emulate_insn()
								vcpu->arch.complete_userspace_io = complete_emulated_pio
kvm_vcpu_ioctl() // ioctl=KVM_RUN
	kvm_arch_vcpu_ioctl_run()
		complete_emulated_pio() // vcpu->arch.complete_userspace_io
			complete_emulated_pio()
				complete_emulated_io()
					kvm_emulate_instruction()
						x86_emulate_instruction()
							x86_emulate_insn()
		vcpu_run()
			vcpu_enter_guest()
				vmx_handle_exit() // RIP 0x000fa591, nest 1, reason IO
					handle_io()
						kvm_emulate_instruction()
							x86_emulate_instruction()
								x86_emulate_insn()
			vcpu_enter_guest()
				vmx_handle_exit() // RIP 0x000fa594, nest 1, reason VMCALL
					...
			...
```

This problem looks too complicated, giving up. Also, it is painful to debug
with print debugging. It may be possible to reproduce this problem with KVM in
KVM, but it would require a complicated setup. For now just report the bug to
KVM.

Using `xmhf64-dev 0596d7e0e` for bug report. Image attached as
`xmhf64-dev-0596d7e0e.img.xz`

Reported KVM bug at <https://bugzilla.kernel.org/show_bug.cgi?id=216234>. Text
saved at `bug_216234.txt`. KVM bugs are tracked in `bug_076`.

### Workaround

In `xmhf64-nest bca582be3`, workaround by hardcoding EPT02. There are code to
hardcode EPT and code to detect REP INS when EPT violation. Only
`0x69000 - 0x7cfff` are observed to be used for REP INS. However, now there
are too many things printed on the screen, and Linux takes too long to boot.

In `xmhf64-nest 1bf106823`, slightly optimize workaround code to make things
run faster. However, Linux is still taking too long to load. One measure
is to use `--enable-optimize-nested-virt` in L2 XMHF (L0 = KVM, L1 = XMHF,
L2 = XMHF, L3 = Linux). This makes VMEXIT caused by L3 a little bit faster.
However, looks like Linux still stucks at WRMSR to `IA32_TSC_DEADLINE`. We
probably need to make XMHF much faster.

### GRUB keyboard problem

In `xmhf64-nest 117389b8f` (L1) and `xmhf64 efdafbd78` (L2), L3 GRUB does not
respond to keyboard events (e.g. press down arrow) correctly. It looks like it
is stuck on VMCALL events forever. The VMCALL event is generated because L2
uses it in E820 handler.

If the code modifying BDA memory in `_vmx_int15_initializehook()` is removed,
GRUB becomes normal, but there are still VMEXITs due to VMCALL, which is
strange.

After some thinking, I realize the problem. Suppose the original BIOS's INT 15h
handler is at 0xf000:0x5678. When L1 XMHF loads, it changes IVT to 0x40:0xac.
The original BIOS handler is saved at 4 bytes starting at 0x4b0. When L2 XMHF
loads, it saves 0x40:0xac to 0x4b0. So when there is a Non-e820 BIOS call, the
L2 XMHF will call 0x4ac forever.

To solve this problem, XMHF should check the value in IVT. If it is already
0x40:0xac, simply skip. Bug fixed in `xmhf64 3d916a673`. However, GRUB is still
very slow in responding to keyboard input. This can be a good way to measure
the efficiency of nested VMEXIT / VMENTRY efficiency.

In `xmhf64-nest bf5a8d0de`, decrease number of VMREAD and VMWRITE during nested
VMREAD / VMWRITE. After this commit, GRUB responds to keyboard events much
faster. However, booting Linux is still slow.

### Running LHV as nested guest

Linux is considered too heavy-weight at this point. So we run
`KVM -> XMHF -> XMHF -> LHV` first. The two XMHF's need to both support nested
virtualization. When single CPU, LHV can run correctly, but is very slow. When
2 or more CPUs, APs see `#GP` when executing VMXON.

This problem happens because XMHF performs INIT-SIPI-SIPI two times during SMP,
see `### INIT two times` in `bug_075`. We can reference code written in
`lhv 10afe107c` (notes in `bug_075`). Fixed in `xmhf64-nest f9d68ee45` by
adding a command line argument. However, looks like this may be a security
vulnerability in XMHF. Need to investigate more in the future.

### SMP XMHF XMHF LHV results in triple fault

When QEMU is configured to run
`KVM -> L1 XMHF (0x10000000) -> L2 XMHF (0x20000000) -> L3 LHV (0x08000000)`, a
triple fault happens when LHV is booting the AP and is captured by L2 XMHF. The
RIP of LHV is `0x10091`, which is `movl %eax, %cr0` in
`bplt-x86-i386-smptrampoline.S`.

In `xmhf64-nest-dev 237af6412`, if we pre-map the following EPT entries in
`xmhf_nested_arch_x86vmx_get_ept02()`, the problem is gone:
* `for (i = 0x00000000ULL; i < 0x02000000ULL; i += PA_PAGE_SIZE_4K)`
* `for (i = 0x08000000ULL; i < 0x09800000ULL; i += PA_PAGE_SIZE_4K)`
* `for (i = 0x1fe00000ULL; i < 0x20000000ULL; i += PA_PAGE_SIZE_4K)`

In `xmhf64-nest-dev bacd45d3a`, only pre-map the EPT entry for `x_3level_pdpt`
in LHV (`0x08b67000` in my case), then the problem is solved.

Looks like this problem is still the `#GP` KVM bug found in `bug_084`. Since AP
sets CR3 quickly and then sets CR0, there is no chance for the original
workaround code to update EPT. We need to think of another way to workaround.
Or maybe we can give up running LHV in a so nested environment. Or just run
64-bits.

The problem disappears when everything runs in 64-bit. Also, in
`xmhf64-nest-dev 143ee52b4`, after removing printf during the following, things
become much more efficient:
* EPT violations
* Nested VMENTRY
* Nexted VMEXIT

So nested XMHF can run LHV well, tested on SMP. Now try running Linux in UP.

After removing printfs, Linux can boot in nested XMHF if everything is i386:
`KVM -smp 1 -> i386 XMHF -> i386 XMHF -> i386 Debian 11`

Note:
* L1 is built with: `./build.sh i386 fast circleci && gr`
* L2 is built with:
  `./build.sh i386 fast --sl-base 0x20000000 circleci --no-init-smp && gr`

Test results:
* Good: `KVM -smp 1 -> i386 XMHF -> i386 XMHF -> i386 Debian 11`
* Good: `KVM -smp 1 -> amd64 XMHF -> i386 XMHF -> i386 Debian 11`
* Good: `KVM -smp 1 -> amd64 XMHF -> amd64 XMHF -> i386 Debian 11`
* Good: `KVM -smp 1 -> amd64 XMHF -> amd64 XMHF -> amd64 Debian 11`
  (need to increase `EPT02_PAGE_POOL_SIZE` from 128 to 192)

When running with multiple cores, Linux seems to be stuck. This is because that
Linux is accessing the APIC EOI register a lot (paddr 0xfee000b0), which
is intercepted by L2 XMHF and hptw library prints a lot of error messages:
```
HPT[3]:...:hptw_checked_get_pmeo:384: EU_CHK( ((access_type & hpt_pmeo_getprot(pmeo)) == access_type) && (cpl == HPTW_CPL0 || hpt_pmeo_getuser(pmeo))) failed
HPT[3]:...:hptw_checked_get_pmeo:384: req-priv:2 req-cpl:0 priv:0 user-accessible:1
HPT[3]:...:hptw_checked_get_pmeo:384: EU_CHK( ((access_type & hpt_pmeo_getprot(pmeo)) == access_type) && (cpl == HPTW_CPL0 || hpt_pmeo_getuser(pmeo))) failed
HPT[3]:...:hptw_checked_get_pmeo:384: req-priv:2 req-cpl:0 priv:0 user-accessible:1
```

I realized that it is possible to use GDB/s `add-symbol-file` to add multiple
symbol files to GDB. This is very helpful when dealing with multiple XMHFs and
Linux. The usage is similar to `symbol-file`.

In `xmhf64 a17801cc6`, `xmhf64-nest 4b50506a3`, `xmhf64-nest-dev 9fcf52cf4`,
reduce hpt and TrustVisor output. However, i386 Linux SMP still cannot boot.

In `xmhf64-nest-dev 353d845fd`, see that there are a huge number of EPT cache
misses. This is caused by LAPIC handling code in XMHF.

```
vmx_lapic_changemapping
	xmhf_memprot_arch_x86vmx_flushmappings
		__vmx_invept(VMX_INVEPT_SINGLECONTEXT, vcpu->vmcs.control_EPT_pointer)
```

This becomes unfortunate, because INVEPT has to invalidate all guest physical
addresses in the EPT. In this case it is incorrect to use INVVPID. One possible
optimization is to cache historical EPT violation addresses, so that when L2
XMHF invalidates EPT, L1 XMHF can reconstruct most of the entries quickly,
without future `L3 -> L1 -> L3` VMEXITs.

Also, another possibility is to use x2APIC, which does not have EPT problems.
Maybe we should start a new bug.

Untried ideas
* Linux stuck on `IA32_TSC_DEADLINE`. Modify the value written by WRMSR or TSC
  multiplier?
	* Not a clean solution, because we do not want to deal with device drivers.
* Try on newer computer
* Support large pages in EPT
* Implement VMCS shadowing

## Fix

`eecb1ffce..1a2b8b266`
* Implement `ept_merge_hpt_pmt()`
* Workaround KVM bugs
* Remove printf code to be faster

