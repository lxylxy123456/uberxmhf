# TrustVisor bugs about registers

## Scope
* All configurations
* TrustVisor
* `xmhf64 b77910ee7`
* `xmhf64-dev 8f516a5d8`

## Behavior
TrustVisor does not clear GPRs and FPU registers. This can lead to leak of
secret values during PAL's computation

## Debugging

`a1.c` can compile on Linux. It uses instructions MOV to read/write ESI and
FLD and FST to read/write FPU values.

In `xmhf64-dev 8f516a5d8..3b8535754`, able to reproduce this vulnerability. By
letting the PAL write secret to EDI, FPU stack, and XMM0, the application can
see the secret after PAL returns.
```sh
./main32 0 3	# EDX	+0xdb962f0f
./main32 1 3	# EDI	+0xfb6d9d22
./main32 2 3	# FPU	+0x411a7502
./main32 3 3	# XMM0	+0x3cbac7fd
```

Bug fixed in `xmhf64-dev 3b8535754..16d8a642d`
* `xmhf64-dev db828f3e7`: clear caller saved GPRs
* `xmhf64-dev 16d8a642d`: disable FPU (set CR0.EM)

Port the fixes to `xmhf64 b77910ee7..78d4ec9ee`

Tested the exploit and fixes on QEMU with (x86 XMHF and x86 Debian) and
(x64 XMHF and x64 Debian), look good.

Also test on Dell 7050 with (x64 XMHF and x64 Debian), look good.

### Backporting to old XMHF

Using `xmhf64-dev 146db3388`, can run the exploit on HP 2540p with Ubuntu 12.04
and old XMHF.

On old XMHF, EDX and EDI (only EDX is bug) exploits are working, but FPU and
XMM0 result in exception 0x7 (#NM). Maybe Ubuntu 12.04 also sets the CR0.EM
flag?

Backport the fix to uberxmhf repo in `xmhf-reg-side-chnl b996e3afd..f7676ec8c`.
Can see that now EDX is always 0 (i.e. bug fix is valid).

Using print debugging, CR0 have two cases (from rich OS `->` PAL)
* `0x8005003b -> 0x8005003f`
* `0x80050033 -> 0x80050037`

That is, `TS = 0 or 1`, `EM = 0 -> 1`, `MP = 1`

I guess the problem that old XMHF cannot reproduce FPU and XMM0 exploit is that
`CR0.TS = 1`. This is a way for OS to lazily avoid saving FPU and SIMD
registers, which causes `#NM` exception at the first time the process accesses
FPU and SIMD registers. I believe our implementation is good.

## Result

`xmhf64 b77910ee7..146db3388`
* Disable FPU and XMM during `scode_marshall()`
* Clear caller saved GPR during `scode_unmarshall()`

`xmhf64-dev 8f516a5d8..146db3388`
* Exploit register side channel / covert channel bug in XMHF64
* Exploit register side channel / covert channel bug in old XMHF

