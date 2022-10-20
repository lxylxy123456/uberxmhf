# Hide VT-d memory addresses

## Scope
* DMAP enabled
* All configurations
* `xmhf64 8b972c159`

## Behavior
Currently XMHF hides the DRHD in `vmx_eap_initialize()` by changing APIC.
However, the DRHD page can still be accessed by the guest if the guest guesses
its address. See comment in `vmx_eap_initialize()`:
```c
411      // zap VT-d presence in ACPI table...
412      // TODO: we need to be a little elegant here. eventually need to setup
413      // EPT/NPTs such that the DMAR pages are unmapped for the guest
414      xmhf_baseplatform_arch_flat_writeu32(dmaraddrphys, 0UL);
```

## Debugging

### Discovery of new vulnerability

Found a possible buffer overflow bug in `vmx_eap_initialize()`: `length` is
read from ACPI memory (untrusted memory), but its not checked.
```c
316      while (i < (dmar.length - sizeof(VTD_DMAR)))
317      {
318          u16 type, length;
319          hva_t remappingstructures_vaddr = (hva_t)remappingstructuresaddrphys;
320  
321          xmhf_baseplatform_arch_flat_copy((u8 *)&type, (u8 *)(remappingstructures_vaddr + i), sizeof(u16));
322          xmhf_baseplatform_arch_flat_copy((u8 *)&length, (u8 *)(remappingstructures_vaddr + i + sizeof(u16)), sizeof(u16));
323  
324          switch (type)
325          {
326          case 0: // DRHD
327              printf("DRHD at %lx, len=%u bytes\n", (remappingstructures_vaddr + i), length);
328              HALT_ON_ERRORCOND(vtd_num_drhd < VTD_MAX_DRHD);
329              xmhf_baseplatform_arch_flat_copy((u8 *)&vtd_drhd[vtd_num_drhd], (u8 *)(remappingstructures_vaddr + i), length);
330              vtd_num_drhd++;
331              i += (u32)length;
332              break;
333  
334          default: // unknown type, we skip this
335              i += (u32)length;
336              break;
337          }
338      }
```

The buffer
overflow can be as large as `sizeof(u16) = 65536` bytes in `.bss` section.
Depending on compile time, it may be close to `g_cpustack`:
```
000000002279b040 b l_vtd_pml4t_paddr
000000002279b048 b vtd_num_drhd
000000002279b060 b vtd_drhd
000000002279b160 B vtd_cet
000000002279c000 B g_cpustacks
000000002281c000 B xmhf_baseplatform_arch_flat_va_offset
```

Why is this bug not caught by formal proof? This code is within
`__XMHF_VERIFICATION__` (skipped due to lack of hardware model)

Fixed in `xmhf64 7acbb46bd`

### Hiding DRHD

The code to hide DRHD registers is implemented in `xmhf64 c3a5816a0`. Then
fixed some compile errors in `xmhf64 c3a5816a0..0a9c369d4`.

DMAP initialization happens only in BSP. So we need a new function to be called
by BSP and APs after SMP initialization. At that time, simply call
`xmhf_memprot_setprot()` to remove DRHD from guest EPT.

To verify, we remove `xmhf_baseplatform_arch_flat_writeu32(dmaraddrphys, 0UL);`
in `vmx_eap_initialize()` that hides VT-d from the guest OS. Can see that after
Debian boots, its access to VT-d causes XMHF to halt. This is expected
behavior.

```
TV[0]:appmain.c:tv_app_handleintercept_hwpgtblviolation:684: CPU(0x00): gva=0xffffffffff240008, gpa=0xfed90008, code=0x1
TV[0]:scode.c:hpt_scode_npf:1352:                  CPU(00): nested page fault!(rip 0xffffffff818ed801, gcr3 0x260a001, gpaddr 0xfed90008, errorcode 1)
TV[3]:scode.c:hpt_scode_npf:1355:                  EU_CHK( hpt_error_wasInsnFetch(vcpu, errorcode)) failed
TV[0]:appmain.c:tv_app_handleintercept_hwpgtblviolation:687: FATAL ERROR: Unexpected return value from page fault handling
```

## Fix

`xmhf64 8b972c159..0a9c369d4`
* Fix buffer overflow in `vmx_eap_initialize()` while reading DRHD
* Protect DRHD from guest memory access

