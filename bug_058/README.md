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

TODO

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

# tmp notes

`--with-opt='-O3 -Wno-stringop-overflow'` for Fedora
`--with-opt='-O3 -Wno-stringop-overflow -Wno-inline'` for Debian

