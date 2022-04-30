# Support Intel Microcode Update

## Scope
* Observed on physical machine only
* Observed on Fedora and Windows 10, not Debian
* Git `1a3f267cd`

## Behavior
The Microcode update is first discovered in `bug_039`. The microcode update
causes page fault, so we disabled the "Hypervisor Present" bit to workaround
the problem. Now we want to support microcode update instead.

## Debugging

Again, we need to read Intel v3 "9.11 MICROCODE UPDATE FACILITIES".
"Table 9-7. Microcode Update Field Definitions" defines the format of microcode
updates, and it looks like we just need to parse the "Data Size" and "Total
Size".

Our plan is
1. Read the total size from guest update data (gva)
2. Copy the update data content (gva) to hypervisor (hva)
3. Compute checksum for the update data (hva), check whether match hard-coded
   value
4. Call microcode update on the copied update data

If any of the above fails, abort.

The main technical problems to solve are
* Be able to walk EPT and guest page table
* Be able to compute checksum

### Review HPT

TrustVisor uses a lot of page table operations. The library it uses is called
"hpt" and "hptw". These probably stand for host page table. They are not
well-documented, so now it's time to add some documentation.

Possible related files are:
* `xmhf/src/libbaremetal/libxmhfutil/hpt.c`:
  operate on primitives (pm, pme)
* `xmhf/src/libbaremetal/libxmhfutil/hpto.c`:
  wrapper for functions in `hpt.c`, operate on pmo and pmeo
* `xmhf/src/libbaremetal/libxmhfutil/hptw.c`:
  wrapper for hpto, operate on `hptw_ctx_t`. Physical address may not be linear.
* `xmhf/src/libbaremetal/libxmhfutil/hpt_internal.c`:
  not built
* `hypapps/trustvisor/src/hptw_emhf.c`:
  code using `hptw.c`

Abbreviations:
* hpt: host page table?
* pm: page map (similar to page table, page directory, ...)
* pme: page map entry (similar to page table entry)
* pmo: page map object (pm + metadata about paging type, current level)
* pmeo: page map entry object (pme + metadata)

We think that `hptw` can be used to perform page walks. We can copy some code
from TrustVisor's `hptw_emhf.c`. However, to debug this we need QEMU and GDB.
Git `67f90bbf4`, see error when walking page table.

### Debugging ucode update

However, the virtual address provided by WRMSR is very strange. We need to
debug Linux source code.

In Fedora, microcode updates are saved in directory
`/usr/lib/firmware/intel-ucode/`.
Should be also available on GitHub:
<https://github.com/intel/Intel-Linux-Processor-Microcode-Data-Files>
Related Linux source code is
<https://elixir.bootlin.com/linux/v5.10.84/source/arch/x86/kernel/cpu/microcode/intel.c>

In Debian, to enable microcode update, need to enable contrib and non-free, and
install `intel-microcode` package. Then in QEMU can break on `load_ucode_bsp()`
(remember to enable nokaslr). Ref: <https://wiki.debian.org/Microcode>

GDB script (without running XMHF)
```
set pagination off
set confirm off
symbol-file usr/lib/debug/boot/vmlinux-5.10.0-9-amd64
directory linux-5.10.84/

hb load_ucode_bsp
c
d

# Test hypervisor present bit, in check_loader_disabled_bsp()
hb *0xffffffff82c4589e
c
p/x $ecx &= 0x7fffffff
d

hb load_ucode_intel_bsp
c
d

# Matching signature in scan_microcode()
hb *0xffffffff8104df59
c
p $eax = 1
d

hb apply_microcode_early
c
d

# WRMSR MSR_IA32_UCODE_WRITE
hb *0xffffffff8104e373
c
```

The above script also works when running XMHF. So we can debug easily.

We see that XMHF's reading of the integers are correct. For example, when
running on QEMU, git `e4905b5d8`, serial output is
```
gva for microcode update: 0xffff88800e0cf2e0
ans[0] = 0x1b26fd03
ans[1] = 0xd21db0c4
ans[2] = 0x87a944f1
ans[3] = 0x8cef80a3
ans[4] = 0x3c3a9518
ans[5] = 0xf997c9a3
ans[6] = 0x8ae424d2
ans[7] = 0xbe26d038
ans[8] = 0x1dabd74d
```

GDB output is
```
(gdb) source gdb/page.gdb 
(gdb) walk_pt 0xffff88800e0cf2e0
$9 = "4-level paging"
$10 = "Page size = 2M"
$11 = 0xe0cf2e0
(gdb) x/10wx 0xe0cf2e0
0xe0cf2e0:	0x1b26fd03	0xd21db0c4	0x87a944f1	0x8cef80a3
0xe0cf2f0:	0x3c3a9518	0xf997c9a3	0x8ae424d2	0xbe26d038
0xe0cf300:	0x1dabd74d	0xbaffdea0
(gdb) x/10bx 0xe0cf2e0
0xe0cf2e0:	0x03	0xfd	0x26	0x1b	0xc4	0xb0	0x1d	0xd2
0xe0cf2e8:	0xf1	0x44
(gdb) 
```

The content can be found in Fedora's `/usr/lib/firmware/intel-ucode`
```
$ hexdump -C 06-0f-02 | grep '03 fd 26 1b c4 b0 1d d2' -U3
00001000  01 00 00 00 5c 00 00 00  10 20 02 10 f2 06 00 00  |....\.... ......|
00001010  0f a6 35 c3 01 00 00 00  20 00 00 00 d0 0f 00 00  |..5..... .......|
00001020  00 10 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00001030  03 fd 26 1b c4 b0 1d d2  f1 44 a9 87 a3 80 ef 8c  |..&......D......|
00001040  18 95 3a 3c a3 c9 97 f9  d2 24 e4 8a 38 d0 26 be  |..:<.....$..8.&.|
00001050  4d d7 ab 1d a0 de ff ba  49 57 21 af ce 4f 73 53  |M.......IW!..OsS|
00001060  b3 f9 8e 83 ca b0 d0 0a  52 c0 a9 e2 31 5e d9 fd  |........R...1^..|
$ 
```

We can see that `06-0f-02` is 8KiB. The first 4 KiB contains an update, the
second 4 KiB contains another. For the microcode update we captured, WRMSR's
EDX:EAX points to the start of "Update Data". We need to decrease by 48 bytes
to get the header.

Git `ef93426f6` prints much more useful information. In HP, `06-25-05` is used.
Serial output for HP is
```
ans[0] = 0x00000001
ans[1] = 0x00000007
ans[2] = 0x04232018
ans[3] = 0x00020655
ans[4] = 0x8b585c97
ans[5] = 0x00000001
ans[6] = 0x00000092
ans[7] = 0x00000fd0
ans[8] = 0x00001000
ans[9] = 0x00000000
ans[10] = 0x00000000
ans[11] = 0x00000000
ans[12] = 0x00000000
ans[13] = 0x000000a1
ans[14] = 0x00020001
ans[15] = 0x00000007
ans[16] = 0x00000000
ans[17] = 0x00000000
ans[18] = 0x20180423
ans[19] = 0x000003e1
ans[20] = 0x00000001
ans[21] = 0x00020655
ans[22] = 0x00000000
ans[23] = 0x00000000
ans[24] = 0x00000000
ans[25] = 0x00000000
ans[26] = 0x00000000
ans[27] = 0x00000000
ans[28] = 0x00000000
```

The head of objdump of `06-25-05` file is
```
00000000  01 00 00 00 07 00 00 00  18 20 23 04 55 06 02 00  |......... #.U...|
00000010  97 5c 58 8b 01 00 00 00  92 00 00 00 d0 0f 00 00  |.\X.............|
00000020  00 10 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000030  00 00 00 00 a1 00 00 00  01 00 02 00 07 00 00 00  |................|
00000040  00 00 00 00 00 00 00 00  23 04 18 20 e1 03 00 00  |........#.. ....|
00000050  01 00 00 00 55 06 02 00  00 00 00 00 00 00 00 00  |....U...........|
00000060  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000090  93 46 f7 1d 2f 08 cf f0  88 b0 8d 49 63 1d a9 40  |.F../......Ic..@|
```

The first 48 bytes form valid header, but after that the encrypted data does
not look indistinguishable from random bytes (e.g. there is clearly
"20180423").

In git `cb987a6b7`, can successfully copy microcode update out to hypervisor
address space.

In git `41199608d`, can successfully trigger the update, and Fedora boots well.
Serial output is:
```
CPU(0x00): date(mmddyyyy)=04232018, dsize=4048, tsize=4096
CPU(0x00): Calling physical ucode update at 0x1d6d0030
CPU(0x00): Physical ucode update returned
CPU(0x04): date(mmddyyyy)=04232018, dsize=4048, tsize=4096
CPU(0x04): Calling physical ucode update at 0x1d6d2030
CPU(0x04): Physical ucode update returned
```

On Windows 10, also works. Serial output is the same.

Git `a98450a7c` makes it a configuration option. From now on, use
`--enable-update-intel-ucode` in `./configure`.

### Checking processor signature

Intel v3 "9.11.3 Processor Identification" says
> Attempting to load a microcode update that does not match a processor
> signature embedded in the
> microcode update with the processor signature returned by CPUID will cause
> the BIOS to reject the update.

However, we probably should not rely on BIOS to detect microcode updates that
should not be loaded.

The checks we need to perform are covered in 
* "9.11.3 Processor Identification": need to check processor signature
* "9.11.4 Platform Identification": need to check processor flag

We do not need to perform
* "9.11.5 Microcode Update Checksum": will be covered by crypto hash

### Check processor signature etc

We implement checking processor identification and platform identification in
git `175c95834`. HP succeeds, QEMU fails. This is expected behavior.

We also use GDB to check QEMU's failure logic. After running the above
"GDB script":

```
d
symbol-file
symbol-file xmhf/src/xmhf-core/xmhf-runtime/runtime.exe
hb handle_intel_ucode_update
c

hb ucode_check_processor
c
```

Unfortunately the `else if (header->total_size > header->data_size + 48)`
branch is not executed. But other things look good.

### Compute crypto hash

We know that TrustVisor uses the crypto library. Likely we need to initialize
something, like in `trustvisor_master_crypto_init()`.

Or maybe we do not need to initialize, just use `sha1_buffer()`.

We verify this experimentally.

It turns out `sha1_buffer()` is very convenient, so we use it. For example,
the following program prints `SHA1={c4f9375f9834b4e7f0a528cc65c055702bf5f24a}`
easily.

```c
unsigned char a[10] = "123456\n";
unsigned char md[SHA_DIGEST_LENGTH];
HALT_ON_ERRORCOND(sha1_buffer(a, 7, md) == 0);
printf("SHA1={");
for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
	printf("%x", md[i]);
}
printf("}");
```

Git `bd96617f2` implements call to `sha1_buffer()`. The serial output is

```
gva for microcode update: 0xffff8880327c7ae0
CPU(0x00): date(mmddyyyy)=04232018, dsize=4048, tsize=4096
SHA1(update) = 48 70 94 85 cb d2 72 9e 96 d8 d0 8e cc 31 77 ad 
SHA1(update) = 57 fd 95 2b 
```

We can verify the result by computing it in Linux
```sh
$ dd if=/usr/lib/firmware/intel-ucode/06-25-05 bs=4K count=1 status=none | sha1sum
48709485cbd2729e96d8d08ecc3177ad57fd952b  -
$
```

### Write Python script to calculate hash

We write a script to parse all updates in `/usr/lib/firmware/intel-ucode` and
compute SHA-1 hashes. See `intel_ucode_sha1.py`.

At this point, commits in `xmhf64-dev` can be diffed through
`18ecd868e..9edbbc2f5`. We squash these commits to `xmhf64`:
`18ecd868e..e995b81e3`.

## Fix

`1a3f267cd..007ab17d2`, `18ecd868e..d0f2be0fb`
* Add comments for the hpt library
* Support forwarding guest's microcode update request to hardware

