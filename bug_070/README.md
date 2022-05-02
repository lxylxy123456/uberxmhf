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

Looks like the problem is TSS. In QEMU, can use `info registers` to print
current registers. Pebbles kernel shows:
`TR =0008 00100950 00000067 00008b00 DPL=0 TSS32-busy`, but LHV shows
`TR =0000 0000000000000000 0000ffff 00008b00 DPL=0 TSS64-busy`.

TODO: check TSS
TODO: try to enable interrupts earlier (e.g. in sl)
TODO: compare with things that are known to work
TODO: write standalone code that can setup IDT
TODO: try on real hardware
TODO: possible to disable KVM and debug QEMU source code

