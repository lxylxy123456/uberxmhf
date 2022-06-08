# XMHF regression on 2540P

## Scope

* i386 XMHF, not tested on amd64
* HP 2540P, not on QEMU
* Git `b32661c70..d8f2220a6`

## Behavior

Miao reported that recently xmhf64 become un-bootable on 2540p. The commit
should be after PR `#8` and before `d8f2220a6`. So it should be
`b32661c70..d8f2220a6`.

The log is like 
```
BSP rallying APs...
BSP(0x00): My ESP is 0x25551000
APs all awake...Setting them free...
AP(0x01): My ESP is 0x25555000, proceeding...
[00]: unhandled exception 13 (0xd), halting!
[00]: error code: 0x00000000
[00]: state dump follows...
[00] CS:EIP 0x0008:0x1021353e with EFLAGS=0x00010006
[00]: VCPU at 0x1024ab60
AP(0x05): My ESP is 0x2555d000, proceeding...
AP(0x04): My ESP is 0x25559000, proceeding...
[00] EAX=0x00000491 EBX=0x1024ab60 ECX=0x00000491 EDX=0x00da0400
[00] ESI=0x1024ab60 EDI=0x1024e49c EBP=0x25550ad4 ESP=0x25550aa8
[00] CS=0x0008, DS=0x0010, ES=0x0010, SS=0x0010
[00] FS=0x0010, GS=0x0010
[00] TR=0x0018
```

## Debugging

Since currently I do not have access to HP 2540P machine. So the plan is to
compile XMHF and send the binary to Miao. Then after seeing the log I can
translate the addresses to symbols.

From the log, it looks like the problem happens on all CPUs soon after calling
`xmhf_runtime_main()`. However we need symbols to get more insight.

The compile commands are (some commands are used with `linksrc.sh`)

```sh
cd .. && umount build32 && linksrc build32 && cd build32
./autogen.sh
./configure '--with-approot=hypapps/trustvisor' '--enable-debug-symbols' '--disable-drt' '--enable-dmap' '--enable-debug-qemu' '--enable-update-intel-ucode'
make -j 4
# gcc (GCC) 11.3.1 20220421 (Red Hat 11.3.1-2)
find -type l -delete
7za a /tmp/d8f2220a6-build32.7z * -mx=1
```

TODO

