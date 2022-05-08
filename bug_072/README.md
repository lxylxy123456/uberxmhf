# Support PAE in i386

## Scope

* i386
* All targets
* Git `a5c055276`

## Behavior

I recently realized that i386 XMHF hypervisor uses PAE paging, so we should be
able to support PAE guests.

## Debugging

First review places that need to be changed.

They key is to look for `MAX_PHYS_ADDR` in amd64 code.

Places where `MAX_PHYS_ADDR` occurs
* `xmhf_sl_arch_x86_setup_runtime_paging()` in `sl-x86.c`: hypervisor PAE
  paging, need to update 4G to `MAX_PHYS_ADDR`
* `_vmx_setupEPT()` in `memp-x86vmx.c`: EPT (no need to change)
* `_vmx_int15_handleintercept()` in `peh-x86vmx-main.c`: do not allow guests
  that have larger physical memory than `MAX_PHYS_ADDR` in i386.

### Current behavior

Currently, running PAE Linux on QEMU with less than 4G memory is fine. However,
when more than 4G memory, see EPT error during boot:

```
...
CPU(0x03): WRMSR (MTRR) 0x000002ff 0x0000000000000000 (old = 0x0000000000000c06)
CPU(0x03): Update EPT memory types due to MTRR
CPU(0x03): WRMSR (MTRR) 0x000002ff 0x0000000000000c06 (old = 0x0000000000000000)
CPU(0x03): Update EPT memory types due to MTRRTV[0]:appmain.c:tv_app_handleintercept_hwpgtblviolation:684: CPU(0x00): gva=0xfffbb000, gpa=0x17fc00000, code=0x2
TV[0]:scode.c:hpt_scode_npf:1328:                  CPU(00): nested page fault!(rip 0xc123130e, gcr3 0x1cae000, gpaddr 0x7fc00000, errorcode 2)
TV[3]:scode.c:hpt_scode_npf:1331:                  EU_CHK( hpt_error_wasInsnFetch(vcpu, errorcode)) failed
TV[0]:appmain.c:tv_app_handleintercept_hwpgtblviolation:687: FATAL ERROR: Unexpected return value from page fault handling
```

In git `5c64c2508`, allow configuring `MAX_PHYS_ADDR` during compilation. Now
PAE boots successfully, but running `pal_demo` fails. This is expected.

```
TV[0]:scode.c:scode_register:458:                  *** scode register ***
TV[0]:scode.c:scode_register:462:                  CPU(0x02): add to whitelist,  scode_info 0xbfb6f9c0, scode_pm 0, gventry 0xbfb6fb1c
TV[0]:scode.c:scode_register:475:                  CR3 value is 0x3634f000, entry point vaddr is 0xb7f4c01d, paddr is 0x1d
HPT[3]:/home/lxy/文档/GreenBox/build32/xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_access_va:391: EU_CHK( ((access_type & hpt_pmeo_getprot(&pmeo)) == access_type) && (cpl == HPTW_CPL0 || hpt_pmeo_getuser(&pmeo))) failed
HPT[3]:/home/lxy/文档/GreenBox/build32/xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_access_va:391: req-priv:1 req-cpl:3 priv:4 user-accessible:0
TV[3]:pt.c:copy_from_current_guest:125:            EU_CHKN( hptw_checked_copy_from_va( &ctx.super, ctx.cpl, dst, gvaddr, len)) failed with: 1
TV[3]:scode.c:parse_params_info:305:               EU_CHKN( copy_from_current_guest(vcpu, &pm_info->num_params, pm_addr, sizeof(pm_info->num_params))) failed with: 1
TV[3]:scode.c:scode_register:478:                  EU_CHKN( parse_params_info(vcpu, &(whitelist_new.params_info), scode_pm)) failed with: 1
```

Now we just need to change `sl-x86.c`.

Then I realized that the problem is that even in PAE paging, it is not possible
to identity map the physical address to virtual address. So we cannot support
PAE accessing large memory easily. If we really want to implement it, we need
to temporarily map the memory when TrustVisor needs to access it.

Git changes that have already been made are `a5c055276..5c64c2508`. They are
in `xmhf64-dev` branch, but not `xmhf64` branch.

## Summary

Cannot implement easily, give up.

