# Optimize XMHF building

## Scope
* All configurations
* Git `ec78a946a`

## Behavior
There are some inefficiencies when building XMHF

## Debugging

The following changes are made
* Do not compile DMAP when DMAP is not enabled
* Do not save pages of zeros in .o files and .exe files

It looks difficult to prevent saving zeros in `runtime.bin`. This is because
multiboot does not support bss in modules (see
<https://www.gnu.org/software/grub/manual/multiboot/multiboot.html>). Note that
multiboot supports bss in the kernel.

... (see `## Fix` below)

Increase in build speed:

* `time ./build.sh amd64 debug`
	* `ec78a946a`: `real 0m22.097s, user 0m32.016s, sys 0m16.915s`
	* `84886fc6f`: `real 0m18.336s, user 0m28.837s, sys 0m14.616s`

### Regression

Found regression between `ec78a946a..84886fc6f`

* `84886fc6f`: bad
* `e2a866a5f`: bad
* `82bf15474`: bad
* `87eb884b3`: bad
* `1fc057b46`: bad
* `8af91ba58`: good
* `ec78a946a`: good

The problem starts from commit `1fc057b46`. Before commit, `runtime.exe`'s
sections are
```
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .text             PROGBITS        0000000010200000 001000 049000 00  AX  0   0 32
  [ 2] .data             PROGBITS        0000000010249000 04a000 42d000 00  WA  0   0 32
  [ 3] .palign_data      NOBITS          0000000010676000 477000 dbd8000 00  WA  0   0 32
  [ 4] .stack            PROGBITS        000000001e24e000 477000 090000 00  WA  0   0 32
```

Since `.stack` is PROGBITS, `runtime.bin` will correctly contain all sections.
However, after commit, `runtime.exe` becomes

```
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .text             PROGBITS        0000000010200000 001000 049000 00  AX  0   0 32
  [ 2] .data             PROGBITS        0000000010249000 04a000 42d000 00  WA  0   0 32
  [ 3] .palign_data      NOBITS          0000000010676000 477000 dbd8000 00  WA  0   0 32
  [ 4] .stack            NOBITS          000000001e24e000 477000 090000 00  WA  0   0 32
```

Now, `runtime.bin` only contains `.text` and `.data`, which causes problems.
XMHF's stack is on OS accessible memory (not protected by EPT etc).

The solution is found in
<https://binutils.sourceware.narkive.com/h5O7g8rR/how-to-expand-the-bss-section-and-fill-it-zeros-when-objcopy-ing>.
Using `--set-section-flags .stack=alloc,load,contents` can force `objcopy` to
fill zeros to `.stack`. This will also cause `.palign_data` to be copied.

### Regression in i386 DRT + DMAP

In git `cd81de2b9`, found regression in i386 DRT + DMAP

* Build command: `./build.sh i386 release`
* Bad serial output:
  `_svm_and_vmx_getvcpu: fatal, unable to retrieve vcpu for id=0x00`
* Expected behavior: should see GRUB the second time

This only happens in i386 and DMAP. Not sure what will happen if no DRT.

Bisect
* `cd81de2b9`: bad
*   `cc879edbc`: bad
*  `84886fc6f`: (need to patch `cc879edbc`)
*    `8af91ba58`: bad
*     `ec78a946a`: bad

Looks like this is not a regression. This happens because
<https://github.com/lxylxy123456/uberxmhf/pull/3> is not merged yet (my memory
is faulty).

### Strange pause in remaking

There is a strange pause when running make in a directory that is already made.

This is because making the hypapp automatically runs `$(APP_ROOT)/autogen.sh`
(e.g. `./hypapps/trustvisor/autogen.sh`). `autoreconf` and `automake` are slow.
Currently I don't have a good way to speed this up. A workaround is to use
`make -j 4`, so that something useful can be done during the waiting time.

## Fix

`ec78a946a..cd81de2b9`
* Avoid compiling DMAP when DMAP is disabled
* Avoid compiling some DRT object files when DRT is disabled
* Avoid saving zeros in .o files and .exe files
* Remove libtommath.a from bootloader (not used)
* Split large build targets for parallelism
* Update .github/build.sh

