# Support Running PALs in DRT

## Scope
* HP, x64 XMHF, DRT
* Good: x86 XMHF
* Good: no DRT
* Git `516e618f6`

## Behavior
When running PALs, the log ends with the following, and then stucks
```
TV[2]:scode.c:scode_register:557:                  skipping scode_clone_gdt
TV[0]:scode.c:scode_register:581:                  generated pme for return gva address 4: lvl:1 5
TV[0]:pages.c:pagelist_get_page:72:                num_used:4 num_alocd:127
TV[0]:pages.c:pagelist_get_page:72:                num_used:5 num_alocd:127
utpm_extend: extending PCR 0
```

In x86, the log should look like:
```
TV[2]:scode.c:scode_register:557:                  skipping scode_clone_gdt
TV[0]:scode.c:scode_register:581:                  generated pme for return gva address 4: lvl:1 5
TV[0]:pages.c:pagelist_get_page:72:                num_used:2 num_alocd:127
utpm_extend: extending PCR 0
utpm_extend: extending PCR 0
utpm_extend: extending PCR 0
utpm_extend: extending PCR 0
TV[0]:appmain.c:tv_app_handleintercept_hwpgtblviolation:662: CPU(0x01): gva=0xb7f8201d, gpa=0xa41c901d, code=0x4
TV[0]:scode.c:hpt_scode_npf:1328:                  CPU(01): nested page fault!(rip 0xb7f8201d, gcr3 0x5e3f000, gpaddr 0xa41c901d, errorcode 4)
TV[0]:scode.c:scode_in_list:122:                   find gvaddr 0xb7f8201d in scode 0 section No.0
TV[0]:scode.c:hpt_scode_switch_scode:1005:         *** to scode ***
```

## Debugging

Looks like we should do print debugging. The call stack is

```
scode_measure_section():217
	utpm_extend():191
		hash_memory_multi():64 (hash_descriptor[hash].process == sha1_process)
			sha1_process()
```

The problem is that the `inlen` argument to `sha1_process()` is extremely large
(e.g. `8444095623094861844 == 0x752f743b00000014`)

The problem is in calling `hash_memory_multi()`. Line 72 is
`curlen = va_arg(args, unsigned long);`. However, the upper 32 bits of `curlen`
is incorrect. Maybe it is more helpful to reproduce this problem on QEMU.

Currently git `e299b8f22`.

In git `08c771c9d`, HP DRT serial `20220320173857`. Tried to invoke
`hash_memory_multi_()` using two functions. `utpm_extend_()` reproduces the
issue, but not `utpm_extend()` does not. This is very strange.

At this exact commit, QEMU (without DRT) can reproduce the problem. In QEMU,
both `utpm_extend()` and `utpm_extend_()` can reproduce.

### Inspect call assembly

The C and asm code are:
```
	rv = hash_memory_multi_(find_hash("sha1"),
							value, &outlen,
							value, TPM_HASH_SIZE,
							mvalue, TPM_HASH_SIZE,
							NULL, NULL);
   0x10238677 <utpm_extend_+96>:	mov    $0x1026e1e0,%edi
   0x1023867c <utpm_extend_+101>:	call   0x1023e434 <find_hash>
   0x10238681 <utpm_extend_+106>:	mov    %eax,%edi
   0x10238683 <utpm_extend_+108>:	lea    -0x38(%rbp),%rsi
   0x10238687 <utpm_extend_+112>:	lea    -0x24(%rbp),%rcx
   0x1023868b <utpm_extend_+116>:	lea    -0x10(%rbp),%rdx
   0x1023868f <utpm_extend_+120>:	lea    -0x24(%rbp),%rax
   0x10238693 <utpm_extend_+124>:	movq   $0x0,0x10(%rsp)
   0x1023869c <utpm_extend_+133>:	movq   $0x0,0x8(%rsp)
   0x102386a5 <utpm_extend_+142>:	movl   $0x14,(%rsp)
   0x102386ac <utpm_extend_+149>:	mov    %rsi,%r9
   0x102386af <utpm_extend_+152>:	mov    $0x14,%r8d
   0x102386b5 <utpm_extend_+158>:	mov    %rax,%rsi
   0x102386b8 <utpm_extend_+161>:	mov    $0x0,%eax
   0x102386bd <utpm_extend_+166>:	call   0x1023bd8d <hash_memory_multi_>
```

Looks like `<utpm_extend_+142>` may need to be `movq`, not `movl`

`utpm.c` is compiled two times, but it is unlikely that the x86 one is linked.
```
./_build_libbaremetal32/_objects/utpm.o
./_build_libbaremetal/_objects/utpm.o
```

This problem is in the object file. Should dive into how `utpm.c` is compiled.

The problem is that `TPM_HASH_SIZE`'s type is `int`, but it is passed to a
variadic function `utpm_extend()` and treated as `unsigned long`.

After fixing this issue, can run PALs well in HP with DRT

At this point, `xmhf64-dev` branch in `b655946ba`.

## Fix

`516e618f6..763ac4121`
* Fix problem in types when calling `hash_memory_multi()`

