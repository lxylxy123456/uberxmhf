# Compiling enhancements

## Scope
* All XMHF
* Git `8867950a1`

## Behavior
XMHF's compile process should be enhanced

## Debugging
Use `include` in Makefile to reduce Makefile logic.

Rename `C_SOURCES32` to `C_SOURCES_BL`.

Write documentation for setting up XMHF64.

Fix compile errors when compiled on Fedora (GCC 11)
* `tpm_extra.c`: using uninitialized stack as random value (security problem?)
* `rijndael.c`: does not specify array length in function argument
* `sha2.c`: does not specify array length in function argument
* `part-x86vmx.c`: strange way to implement `get_cpu_vendor_or_die()`
* Fix linker scripts
	* New debug sections: `.debug_line_str`, `.debug_rnglists`
	* New note section: `.note.gnu.property`
	* New note section: `.gnu.build.attributes`, see
	  <https://fedoraproject.org/wiki/Toolchain/Watermark>

Now at `2bd0b9800`, Fedora can compile x64 XMHF. The configuration options is
the same as Debian.

```sh
sudo dnf install autoconf automake make gcc
```

Cross compiling is also easier. There is no strange issue in Debian that
requires overriding `CC` and `LD`. e.g.
```sh
./configure --with-approot=hypapps/trustvisor --enable-debug-symbols --enable-debug-qemu --disable-dmap --disable-drt
```

## Fix

`8867950a1..c07296815`
* Define runtime.mk to capture common logic in runtime Makefiles
* Write documentation for setting up XMHF64
* Fix compile errors when compiling on Fedora
* Add documentations for compiling on Fedora

