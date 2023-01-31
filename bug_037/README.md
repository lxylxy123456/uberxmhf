# Enable DRT (DTRM)

## Scope
* HP x64
* Good: HP x86
* Maybe not applicable to QEMU, because need TXT support

## Behavior
Currently DRTM is turned off using `--disable-drt`. Hopefully after enabling
DRT XMHF should still work in x86. However this has not been tested for a long
time. Also supporting DRT in x64 should not be too complicated, because it
looks like most of the work happens in `bootloader -> secureloader`.

## Debugging

Some background materials related to DRTM, SRTM, TXT:
* <https://trustedcomputinggroup.org/wp-content/uploads/DRTM-Specification-Overview_June2013.pdf>
* <https://trustedcomputinggroup.org/resource/d-rtm-architecture-specification/>
* <https://security.stackexchange.com/questions/53258/>
* <https://security.stackexchange.com/questions/39329/>

### Compile errors

First fixed some compile errors due to data types. Now git at `d4ef00c7a`,
x86 XMHF serial `20220127145105`, x64 XMHF serial `20220127145132`.

In x64, HP automatically reboots (looks like triple fault).

The following line numbers are for
<https://github.com/lxylxy123456/uberxmhf/blob/d4ef00c7acc1a14b2e57c2cfe9672065c9265153/xmhf/src/xmhf-core/xmhf-secureloader/arch/x86_64/sl-x86_64-entry.S>

* If we add an infinite loop after `_sl_start` (line 110), reboot no longer
  happens.
* Add infinite loop before `enable paging` (line 189), no reboot
* Add infinite loop in x64 mode (line 201), reboot
* Add infinite loop after move to CR0 (line 194), reboot

From our previos experience, we are basically certain that this is a triple
fault when enabling long mode. Unfortunately it looks like QEMU cannot run TXT.

### Check CS

Start branch `xmhf64-drt` to store debugging code.

Git `bde35cb55` can print simple things in sl entry code. HP without DRTM
shows:
```
CS=0x00000010
DS=0x00000018
ES=0x00000018
FS=0x00000018
EBP=0x10000000
GDT0=0xb8a0002f	GDT1=0x00001000
GDT2=0x00000000	GDT3=0x00000000
GDT4=0x00000000	GDT5=0x00000000
GDT6=0x0000ffff	GDT7=0x00cf9a00
GDT8=0x0000ffff	GDT9=0x00af9a00
GDTa=0x0000ffff	GDTb=0x00cf9200
GDTc=0x00000000	GDTd=0x00000000
GDTe=0x00000000	GDTf=0x00000000
```

HP with DRTM shows:
```
CS=0x00000008
DS=0x00000010
ES=0x00000018
FS=0x00000018
EBP=0x10000000
GDT0=0x00000000	GDT1=0x30040004
GDT2=0x0004f04b	GDT3=0xc08b0078
GDT4=0x52494350	GDT5=0x00000000
GDT6=0x020004f0	GDT7=0x30303030
GDT8=0x00000080	GDT9=0x00000800
GDTa=0x64300400	GDTb=0x42493030
GDTc=0x00000000	GDTd=0x0004f048
GDTe=0xf0496430	GDTf=0x656c6269
```

We can see that CS and DS are different. This also causes GDT to be garbled.
However it may not be easy to determine CS and DS offset.

For simplicity in development we can adapt the code in `sl-x86-entry.S` to
first establish a GDT where all base addresses are 0. Then jump to long mode.

### Segments

Note that EBP is expected, so EIP should also be expected.

After some experiments using git `837bcbda2`, we can see that
```
                         DRT=y      | DRT=n
expected               = 0x00cf9a00 | 0x00cf9a00
    sl_gdt+0x7*4(%ebp) = 0x656c6269 | 0x00cf9a00
%cs:sl_gdt+0x7*4(%ebp) = 0x00cf9a00 | 0x00cf9a00
%ds:sl_gdt+0x7*4(%ebp) = 0x00cf9a00 | 0x00cf9a00
%es:sl_gdt+0x7*4(%ebp) = 0x00000000 | 0x00cf9a00
%fs:sl_gdt+0x7*4(%ebp) = 0x00cf9a00 | 0x00cf9a00
%ss:sl_gdt+0x7*4(%ebp) = 0x020004f0 | 0x00cf9a00
```

Now we can better understand the "Cross-processor challenge:" comments in
`sl-x86-entry.S`.

> 1. On AMD CS,SS are valid and DS is not.
> 2. On Intel CS,DS are valid and SS is not.

The line above means that using CS and DS to access memory will create the
correct result (i.e. base address = 0). When using other segments (or when not
using segments), get incorrect result.

> 3. On both, CS is read-only.

The line above means that memory accesses in the form of `%cs:(%eax)` cannot be
writing. However I have not tested it yet.

So it looks like that if we change all memory access to use `%cs` and `%ds`,
the problem should be solved. If we need to support AMD in the future, need to
test CPUID and use `%ss` instead.

In git `23ae7b4b7`, we can load CR0 correctly (no triple fault now).

We also found that `leal (%ebp)` is the same as `leal %Xs:(%ebp)`, where `X` is
any segment register. We are not using segment registers here. In x86 mode
code, looks like we need to assume that EBP is the physical address of SL. So
we also assume this in x64 mode.

If we remove the infinite loop, x64 code can start running (see printf output).

Previous Ideas
* Check whether CS becomes unexpected value
	* Checked, and CS becomes unexpected
* Check whether gdt data is loaded correctly
	* Checked, GDT data is not loaded correctly, and fixed
* Make SL position independent
	* Looks like actually not required. Ignored
* Can reuse some x86 SL code
	* Will reuse when need to support AMD

### ACPI problem

After fixing the above problem, git in `5461d4844`, serial tail is the
following and then the machine reboots automatically:
```
SL: RPB, magic=0xf00ddead
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI type-1 access supported.
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI bus enumeration follows:
xmhf_baseplatform_arch_x86_64_pci_initialize: Done with PCI bus enumeration.
```

The call stack should be:
```
xmhf_sl_main
	xmhf_baseplatform_initialize
		xmhf_baseplatform_arch_x86_64_pci_initialize
			(successfully return)
		xmhf_baseplatform_arch_x86_64_acpi_getRSDP(&rsdp)
			(triple fault)
```

We can see that `_acpi_computetablechecksum()` causes the problem.

Expected (i.e. without DRT) (when working on `bug_042`: this is wrong)
```
LINE 110
signature=0x2052545020445352
checksum =0x8f
oemid[0] =0x48
oemid[1] =0x50
oemid[2] =0x51
oemid[3] =0x4f
oemid[4] =0x45
oemid[5] =0x4d
revision =0x02
rsdtaddress =0xbb3fe0ac
length   =0x00000024
xsdtaddress =0x00000000bb3fe120
xchecksum =0xe1
LINE 124
LINE 64 0x1d9f7f84 0x00000014
LINE 66 0x1d9f7f84
...
LINE 66 0x1d9f7f97
LINE 126
LINE 133
LINE2 99
```

Actual:
```
LINE 110
signature=0x2052545020445352
checksum =0x8f
oemid[0] =0x48
oemid[1] =0x50
oemid[2] =0x51
oemid[3] =0x4f
oemid[4] =0x45
oemid[5] =0x4d
revision =0x02
rsdtaddress =0xbb3fe0ac
length   =0x00000024
xsdtaddress =0x00000000bb3fe120
xchecksum =0xe1
LINE 124
LINE 64 0x0000ff7c 0x00000014
LINE 66 0x0000ff7c
...
LINE 66 0x0000ff8b
LINE 66 0x0000ff8c
LINE 66 0x0000ff8d
LINE 66 0x0000ff
```

Will fix in `bug_042`.

## Fix

`835d832f6..a2fe5a973`
* Fix compile errors due to data types
* Use DS to access memory to work in Inel TXT mode

