# Support changing MTRRs

## Scope
* All configurations
* Problematic in HP GRUB graphics mode / Windows 7
* Git `1e456c131`

## Behavior
Windows 7 and GRUB graphical mode need to edit MTRRs. However currently XMHF
does not support it. For example, booting Windows 7 will result in serial:
```
CPU(0x00): Old       MTRR 0x00000200 is 0x 00000000 ffc00005
CPU(0x00): Modifying MTRR 0x00000200 to 0x 00000000 00000006
CPU(0x00): Modifying MTRR not yet supported. Halt!
```

## Debugging

Terminologies
* HPT = EPT in Intel = nested page map (NPM)
* GPT =                guest page map (GPM)

Sample use of EPT is in `scode_lend_section()`.

```
284      /* revoke access from 'reg' VM */
285      hpt_pmeo_setprot(&page_reg_npmeo, section->reg_prot);
286      hpt_err = hptw_insert_pmeo(reg_npm_ctx,
287                                     &page_reg_npmeo,
288                                     page_reg_gpa);
289      CHK_RV(hpt_err);
290  
         ...
302  
303      /* add access to pal nested page tables */
304      page_pal_npmeo = page_reg_npmeo;
305      hpt_pmeo_setprot(&page_pal_npmeo, section->pal_prot);
306      hpt_err = hptw_insert_pmeo_alloc(pal_npm_ctx,
307                                           &page_pal_npmeo,
308                                           page_pal_gpa);
309      CHK_RV(hpt_err);
```

Can see that `page_reg_npmeo` contains the original MTRR settings, then is
copied to `page_pal_npmeo`, and applied to the PAL.

The EPT structure is defined in Intel v3
"Table 27-6. Format of an EPT Page-Table Entry that Maps a 4-KByte Page".
* Bit 0-2 are rwx access (interested by TrustVisor)
* Bit 3-5 are cache settings (equivalent to MTRR)

Due to the way PALs modify EPTs, I believe it is necessary for XMHF to notify
hypapps of changes in EPT.

### Change code

Related files are:
* In `peh-x86vmx-main.c`, `_vmx_handle_intercept_wrmsr()` is the MTRR handler
* In `memp-x86vmx.c`, `_vmx_setupEPT()` sets up EPT, and
  `_vmx_getmemorytypeforphysicalpage()` is used to understand MTRRs.
* In hypapps, place report function near `xmhf_app_main()`

Findings when modifying the code
* It is possible to use a variable range MTRR to create multiple non-continuous
  ranges (though discouraged by Intel), so current MTRR handling code has to be
  rewritten.
* Linux also modifies `IA32_MTRR_DEF_TYPE`: `arch/x86/kernel/cpu/mtrr/generic.c`
  (search for `mtrr_wrmsr()` and `MSR_MTRRdefType`). Previous code did not
  catch WRMSR to `IA32_MTRR_DEF_TYPE`. This becomes another reason to support
  MTRRs.

# tmp notes

