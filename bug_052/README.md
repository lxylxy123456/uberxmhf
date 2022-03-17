# Possible regression in DMAP due to `bug_049`

## Scope
* DMAP enabled
* Git `e82b0f5fd`

## Behavior
The error of DMAP changes before and after `bug_049`.

Before, the 2nd time GRUB can load, and during OS the VGA is broken

After, for x64 see
```
vmx_eap_initialize: initializing DRHD unit 1...
	VT-d hardware Snoop Control (SC) unavailable
	VT-d haThe system is powered off.
The system is powered on.
```

For x32, see
```
	Checking and disabling PMR...Done.
vmx_eap_initialize: success, leaving...
_svm_and_vmx_getvcpu: fatal, unable to retrieve vcpu for id=0x00
```

## Debugging

Compile should use

```
linksrc .
builder
make clean

./build.sh amd64 release

./autogen.sh
./configure --with-approot=hypapps/trustvisor --disable-drt --enable-debug-symbols --enable-debug-qemu CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld
make -j 4
mv hypervisor-x86.bin.gz hypervisor-x86-i386.bin.gz
mv init-x86.bin init-x86-i386.bin
copy32 && hpinit6

./autogen.sh
./configure --with-approot=hypapps/trustvisor --disable-drt --with-target-hwplatform=x86_64 --enable-debug-symbols --enable-debug-qemu
make -j 4
mv hypervisor-x86_64.bin.gz hypervisor-x86-amd64.bin.gz
mv init-x86_64.bin init-x86-amd64.bin
copy64 && hpinit6
```

For x86, before and after `bug_049` have the same error:
```
_vmx_getvcpu: fatal, unable to retrieve vcpu for id=0x00
```

For x64, before `bug_049` is broken VGA (considered "good" in bisect).
If cannot see grub the 2nd time, considered "bad" in bisect

First bisect using x64.

* `f0baec0bb`: good
* `65a3f5ea4`: bad? == f8f0a50e4
* `f8f0a50e4`: good
* `e4ff5ad03`: good
* `ac4a81ad3`: good
* `049e08980`: good
* `6039c0471`: good
* `896edafb9`: good?
* `0240f2b7a`: good?

Looks like everything is good?

## Result

Actually not a regression, but I was confused during bisecting.

For x64:
* When DRT and DMAP is enabled, will see the "bad" behavior, because DRT will
  cause triple fault.
* When only DMAP is enabled, will see the "good" behavior.

