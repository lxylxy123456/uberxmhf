# Support compile with optimization

## Scope
* All XMHF
* Git `c07296815`

## Behavior
XMHF should be able to compile with `-O3`

## Debugging

First add configuration option to support setting optimization level.

The new configuration command is
```
./autogen.sh
./configure --with-approot=hypapps/trustvisor --disable-drt --disable-dmap --with-opt=-O3
```

### Warning `-Wstringop-overflow`

One problem found related to `-Wstringop-overflow` is, for example,
```c
void draw_vga(void) {
	char *vgamem = (char *)0xB8000;
	vgamem[0]= 0x0f;
}
```

The error message is
`writing 1 byte into a region of size 0 [-Werror=stringop-overflow=]`. The fix
is to make the pointer `volatile`. i.e.
```c
void draw_vga(void) {
	volatile char *vgamem = (volatile char *)0xB8000;
	vgamem[0]= 0x0f;
}
```

### Strange compile error in `tmp_extra.c`

Get a strange "writing 1 byte into a region of size 0" error, but not sure why
this happens. Guessing that compiler has a problem.

Git `437d519a9..6930171dd` reduce the code to a single file. Copied to `a.c`.

Likely a GCC bug. Not sure whether is a duplicate of existing bugs, so did not
report bug. Woraround is to use `--with-opt='-O3 -Wno-stringop-overflow'` to
skip this error.

This is also reproducible in Debian 11.

Bug report written in `bug.txt`, attachment is `b.i`. Reported in
<https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105100>

TODO

### Compile error in Debian 11's environment

This bug happens in Debian 11's gcc version 10, but not Fedora's gcc version 11.
```
scode.c: In function 'hpt_scode_switch_regular':
xmhf/src/xmhf-core/include/arch/x86/xmhf-baseplatform-arch-x86.h:534:20: error: inlining failed in call to 'VCPU_gnmiblock_set': call is unlikely and code size would grow [-Werror=inline]
  534 | static inline void VCPU_gnmiblock_set(VCPU *vcpu, int block)
      |                    ^~~~~~~~~~~~~~~~~~
scode.c:1281:5: note: called from here
 1281 |     VCPU_gnmiblock_set(vcpu, 0);
      |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~
cc1: all warnings being treated as errors
```

This is fixed by git `08fcdb596..98b48c6aa`, which adds unlikely compiler hint
in `EU_CHK` macros (unlikely is implemented with `__builtin_expect(!!(x), 0)`,
from Linux code).

### Linking error

Error shows that `.unaccounted` contains information. Cherry pick git
`72d77dff3` to temporarily remove `.unaccounted` sections.

The 2 types of new sections related to code are:
* `.text.unlikely`
* `.rodata.str1.*` (e.g. `.rodata.str1.8`), see
  <https://stackoverflow.com/questions/47323981/>

The 2 types of new section related to debug info is:
* `.debug_loc` (Found in Debian 11)
* `.debug_loclists` (Found in Fedora 35)

### Runtime error: APs stuck at SMP initialize

In `xmhf_baseplatform_arch_x86_smpinitialize_commonstart()`, APs stuck at
`while(!g_ap_go_signal);` loop. The compiler optimizes this loop to a JMP
instruction. This is because the `g_ap_go_signal` variable should be
`volatile`.

Updated the following variables:
* `g_cpus_active`
* `g_lock_cpus_active`
* `g_ap_go_signal`
* `g_lock_ap_go_signal`
* `g_vmx_quiesce_counter`
* `g_vmx_lock_quiesce_counter`
* `g_vmx_quiesce_resume_counter`
* `g_vmx_lock_quiesce_resume_counter`
* `g_vmx_quiesce `
* `g_vmx_lock_quiesce`
* `g_vmx_quiesce_resume_signal`
* `g_vmx_lock_quiesce_resume_signal`
* `g_vmx_flush_all_tlb_signal`
* `vcpu->sipireceived`
* `g_appmain_success_counter`
* `g_lock_appmain_success_counter`

See git `b94f3aa19..7a41602ef`

After the change, QEMU and HP XMHF 32 can run successfully.

### Testing

| XMHF | DRT | OS             | Platform | App          | Result |
|------|-----|----------------|----------|--------------|--------|
| x86  | n   | Debian 11 x86  | QEMU     | pal_demo x86 | good   |
| x86  | n   | Debian 11 x86  | HP       | pal_demo x86 | good   |
| x86  | y   | Debian 11 x86  | HP       | pal_demo x86 | good   |
| x64  | n   | Debian 11 x64  | QEMU     | pal_demo *   | good   |
| x64  | n   | Debian 11 x64  | HP       | pal_demo *   | good   |
| x64  | y   | Debian 11 x64  | HP       | pal_demo *   | good   |
| x64  | y   | Windows 10 x64 | HP       | pal_demo *   | good   |

Looks well, so marking this bug as fixed.

### Conclusion

There are not a lot of things done to support `-O3` compile. Basically fixed
some compile warnings, and use `--with-opt='-O3 -Wno-stringop-overflow'` to
workaround a compile warning that cannot be fixed. The most important runtime
problem is using `volatile` on variables to be accessed by multiple CPUs
concurrently.

## Fix

`430f6881b..822842a83`
* Fix compile warnings / errors when compile with `-O3`
* Add ELF sections to linker scripts when compile with `-O3`
* Use `volatile` keyword to fix runtime problems when `-O3`

