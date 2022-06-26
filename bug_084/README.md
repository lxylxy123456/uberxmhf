# Support guest hypervisor setting EPT

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `acad9ef02`
* Git branch `lhv` commit `d6eea8116`

## Behavior
Currently for the guest hypervisor running in XMHF, EPT is not supported.
This need to be changed in order to run unrestricted nested guests.

## Debugging

First try to remove bss from XMHF's compressed image, at least in build option.
In lhv `6ff70ad37`, xmhf64-nest `2e2a3c300`, the build and run time is
decreased.

First use EPT in lhv.

Using the hpt library to construct EPT automatically. When compiling LHV,
encountered strange error during linking. In xmhf64 `8b5de43bf`, removed
`hpt_internal.c` (duplicate content compared to `hpt.c`).

In lhv `7128d975d`, EPT looks like

```
(gdb) x/gx ept_root
0x8bb4000:	0x0000000008bbc007
(gdb) ...
0x8bbc000:	0x0000000008bbd007
(gdb) ...
0x8bbd040:	0x0000000000000000	0x0000000008bbe007
(gdb) ...
0x8bbe1a0:	0x0000000001234007	0x0000000000000000
(gdb) 
```

My code is following TrustVisor's `scode.c`. So this should also be
TrustVisor's behavior. However, note that the EPT's memory type is set to UC
(uncached), which is not good.

Now we debug TrustVisor.

Break at `scode_register()`, and trigger `pal_demo` in Debian.

Can see that before entering TrustVisor, memory type is WB (6).
```
(gdb) p/x vcpu->vmcs.control_EPT_pointer 
$1 = 0x1467401e
(gdb) x/2gx 0x14674000
0x14674000 <g_vmx_ept_pml4_table_buffers>:	0x000000001467c007	0x0000000000000000
(gdb) x/2gx 0x000000001467c000
0x1467c000 <g_vmx_ept_pdp_table_buffers>:	0x0000000014684007	0x0000000014685007
(gdb) x/2gx 0x0000000014684000
0x14684000 <g_vmx_ept_pd_table_buffers>:	0x00000000146a4007	0x00000000146a5007
(gdb) x/2gx 0x00000000146a4000
0x146a4000 <g_vmx_ept_p_table_buffers>:	0x0000000000000037	0x0000000000001037
(gdb) 
```

Then break at `hpt_scode_switch_scode()` and wait until this function ends.
However, the memory types are correct (WB, 6).

```
(gdb) p/x vcpu->vmcs.control_EPT_pointer 
$10 = 0x1030e01e
(gdb) x/2gx 0x1030e000
0x1030e000:	0x000000001030f007	0x0000000000000000
(gdb) ...
0x1030f000:	0x0000000010310007	0x0000000000000000
...
0x10310250:	0x0000000010313007	0x0000000000000000
...
0x10313850:	0x000000000950a035	0x0000000000000000
...
0x10313930:	0x0000000009526033	0x0000000000000000
0x10313940:	0x0000000000000000	0x0000000009529033
(gdb) 
```

I guess the EPT pages are copied from the large EPT, so the memory type bits
are not changed. I also realized that `hpt_pme_set_pmt()` and
`hpt_pmeo_setcache()` can be used to set the memory type to WB.

Now the memory type is correct:

```
0x8bbe1a0:	0x0000000001234037	0x0000000000000000
```

Also encountered triple fault after enabling EPT in i386 lhv. The cause is the
same as in `bug_079`. Since i386 LHV uses PAE, the PDPTE needs to be populated.

TODO

