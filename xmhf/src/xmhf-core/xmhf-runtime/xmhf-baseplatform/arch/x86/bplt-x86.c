/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

/*
 * EMHF base platform component interface, x86 common backend
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf.h>

//get CPU vendor
u32 xmhf_baseplatform_arch_getcpuvendor(void){
	u32 vendor_dword1, vendor_dword2, vendor_dword3;
	u32 cpu_vendor;
	asm(	"xor	%%eax, %%eax \n"
					"cpuid \n"
					"mov	%%ebx, %0 \n"
					"mov	%%edx, %1 \n"
					"mov	%%ecx, %2 \n"
					 : "=m"(vendor_dword1), "=m"(vendor_dword2), "=m"(vendor_dword3)
					 : //no inputs
					 : "eax", "ebx", "ecx", "edx" );

	if(vendor_dword1 == AMD_STRING_DWORD1 && vendor_dword2 == AMD_STRING_DWORD2
			&& vendor_dword3 == AMD_STRING_DWORD3)
		cpu_vendor = CPU_VENDOR_AMD;
	else if(vendor_dword1 == INTEL_STRING_DWORD1 && vendor_dword2 == INTEL_STRING_DWORD2
			&& vendor_dword3 == INTEL_STRING_DWORD3)
		cpu_vendor = CPU_VENDOR_INTEL;
	else{
		printf("%s: unrecognized x86 CPU (0x%08x:0x%08x:0x%08x). HALT!\n",
			__FUNCTION__, vendor_dword1, vendor_dword2, vendor_dword3);
		HALT();
	}

	return cpu_vendor;
}


//initialize basic platform elements
void xmhf_baseplatform_arch_initialize(void){
	//initialize PCI subsystem
	xmhf_baseplatform_arch_x86_pci_initialize();

	//check ACPI subsystem
	{
		ACPI_RSDP rsdp;
		#ifndef __XMHF_VERIFICATION__
			//TODO: plug in a BIOS data area map/model
			if(!xmhf_baseplatform_arch_x86_acpi_getRSDP(&rsdp)){
				printf("%s: ACPI RSDP not found, Halting!\n", __FUNCTION__);
				HALT();
			}
		#endif //__XMHF_VERIFICATION__
	}

}


//initialize CPU state
void xmhf_baseplatform_arch_cpuinitialize(void){
	u32 cpu_vendor = xmhf_baseplatform_arch_getcpuvendor();

	//set OSXSAVE bit in CR4 to enable us to pass-thru XSETBV intercepts
	//when the CPU supports XSAVE feature
	if(xmhf_baseplatform_arch_x86_cpuhasxsavefeature()){
		u32 t_cr4;
		t_cr4 = read_cr4();
		t_cr4 |= CR4_OSXSAVE;
		write_cr4(t_cr4);
	}

	if(cpu_vendor == CPU_VENDOR_INTEL)
		xmhf_baseplatform_arch_x86vmx_cpuinitialize();
}

u64 VCPU_gdtr_base(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_GDTR_base);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_GDTR_base;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->gdtr.base;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

size_t VCPU_gdtr_limit(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmread32(VMCSENC_guest_GDTR_limit);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_GDTR_limit;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->gdtr.limit;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

u64 VCPU_grflags(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_RFLAGS);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_RFLAGS;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->rflags;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_grflags_set(VCPU *vcpu, u64 val)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_RFLAGS, val);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_RFLAGS = val;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->rflags = val;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

u64 VCPU_grip(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_RIP);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_RIP;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->rip;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_grip_set(VCPU *vcpu, u64 val)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_RIP, val);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_RIP = val;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->rip = val;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

u64 VCPU_grsp(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_RSP);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_RSP;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->rsp;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_grsp_set(VCPU *vcpu, u64 val)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_RSP, val);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_RSP = val;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->rsp = val;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

ulong_t VCPU_gcr0(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_CR0);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_CR0;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->cr0;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_gcr0_set(VCPU *vcpu, ulong_t cr0)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_CR0, cr0);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_CR0 = cr0;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->cr0 = cr0;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

u64 VCPU_gcr3(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_CR3);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_CR3;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->cr3;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_gcr3_set(VCPU *vcpu, u64 cr3)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_CR3, cr3);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_CR3 = cr3;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->cr3 = cr3;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

ulong_t VCPU_gcr4(VCPU *vcpu)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      return __vmx_vmreadNW(VMCSENC_guest_CR4);
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    return vcpu->vmcs.guest_CR4;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    return vcpu->vmcb_vaddr_ptr->cr4;
  } else {
    HALT_ON_ERRORCOND(false);
    return 0;
  }
}

void VCPU_gcr4_set(VCPU *vcpu, ulong_t cr4)
{
  if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
    if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
      __vmx_vmwriteNW(VMCSENC_guest_CR4, cr4);
      return;
    }
#endif /* __NESTED_VIRTUALIZATION__ */
    vcpu->vmcs.guest_CR4 = cr4;
  } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
    vcpu->vmcb_vaddr_ptr->cr4 = cr4;
  } else {
    HALT_ON_ERRORCOND(false);
  }
}

/* Return whether guest OS is in long mode (return 1 or 0) */
bool VCPU_glm(VCPU *vcpu) {
    if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
        if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
            return (__vmx_vmread32(VMCSENC_control_VM_entry_controls) >>
                    VMX_VMENTRY_IA_32E_MODE_GUEST) & 1U;
        }
#endif /* __NESTED_VIRTUALIZATION__ */
        return (vcpu->vmcs.control_VM_entry_controls >>
                VMX_VMENTRY_IA_32E_MODE_GUEST) & 1U;
    } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
        /* Not implemented */
        HALT_ON_ERRORCOND(false);
        return false;
    } else {
        HALT_ON_ERRORCOND(false);
        return false;
    }
}

/*
 * Return whether guest application is in 64-bit mode (return 1 or 0).
 * If guest OS is in long mode, return 1 if guest application in 64-bit mode.
 * If guest OS in legacy mode (e.g. protected mode), will always return 0;
 */
bool VCPU_g64(VCPU *vcpu) {
    if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
        if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
            return (__vmx_vmread32(VMCSENC_guest_CS_access_rights) >> 13) & 1U;
        }
#endif /* __NESTED_VIRTUALIZATION__ */
        return (vcpu->vmcs.guest_CS_access_rights >> 13) & 1U;
    } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
        /* Not implemented */
        HALT_ON_ERRORCOND(false);
        return false;
    } else {
        HALT_ON_ERRORCOND(false);
        return false;
    }
}

/*
 * Update vcpu->vmcs.guest_PDPTE{0..3} for PAE guests. This is needed
 * after guest CR3 is changed.
 */
void VCPU_gpdpte_set(VCPU *vcpu, u64 pdptes[4]) {
    if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
        if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
            __vmx_vmwrite64(VMCSENC_guest_PDPTE0, pdptes[0]);
            __vmx_vmwrite64(VMCSENC_guest_PDPTE1, pdptes[1]);
            __vmx_vmwrite64(VMCSENC_guest_PDPTE2, pdptes[2]);
            __vmx_vmwrite64(VMCSENC_guest_PDPTE3, pdptes[3]);
            return;
        }
#endif /* __NESTED_VIRTUALIZATION__ */
        vcpu->vmcs.guest_PDPTE0 = pdptes[0];
        vcpu->vmcs.guest_PDPTE1 = pdptes[1];
        vcpu->vmcs.guest_PDPTE2 = pdptes[2];
        vcpu->vmcs.guest_PDPTE3 = pdptes[3];
    } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
        /* Not implemented */
        HALT_ON_ERRORCOND(false);
    } else {
        HALT_ON_ERRORCOND(false);
    }
}

/*
 * Return whether guest is running in L2 mode, i.e. VMX non-root. When nested
 * virtualization is not enabled, always return false.
 */
bool VCPU_nested(VCPU *vcpu) {
    if (vcpu->cpu_vendor == CPU_VENDOR_INTEL) {
#ifdef __NESTED_VIRTUALIZATION__
        if (vcpu->vmx_nested_operation_mode == NESTED_VMX_MODE_NONROOT) {
            return true;
        }
#endif /* __NESTED_VIRTUALIZATION__ */
        return false;
    } else if (vcpu->cpu_vendor == CPU_VENDOR_AMD) {
        /* Not implemented */
        HALT_ON_ERRORCOND(false);
        return false;
    } else {
        HALT_ON_ERRORCOND(false);
        return false;
    }
}

/*
 * Get a guest register
 */
ulong_t VCPU_reg_get(VCPU *vcpu, struct regs* r, enum CPU_Reg_Sel sel)
{
    switch (sel)
    {
#ifdef __AMD64__
        case CPU_REG_AX: return r->rax;
        case CPU_REG_CX: return r->rcx;
        case CPU_REG_DX: return r->rdx;
        case CPU_REG_BX: return r->rbx;
        /* CPU_REG_SP is managed by VCPU_grsp() */
        case CPU_REG_BP: return r->rbp;
        case CPU_REG_SI: return r->rsi;
        case CPU_REG_DI: return r->rdi;
        case CPU_REG_R8: return r->r8;
        case CPU_REG_R9: return r->r9;
        case CPU_REG_R10: return r->r10;
        case CPU_REG_R11: return r->r11;
        case CPU_REG_R12: return r->r12;
        case CPU_REG_R13: return r->r13;
        case CPU_REG_R14: return r->r14;
        case CPU_REG_R15: return r->r15;
#elif defined(__I386__)
        case CPU_REG_AX: return r->eax;
        case CPU_REG_CX: return r->ecx;
        case CPU_REG_DX: return r->edx;
        case CPU_REG_BX: return r->ebx;
        /* CPU_REG_SP is managed by VCPU_grsp() */
        case CPU_REG_BP: return r->ebp;
        case CPU_REG_SI: return r->esi;
        case CPU_REG_DI: return r->edi;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

        case CPU_REG_SP: return VCPU_grsp(vcpu);
        case CPU_REG_FLAGS: return VCPU_grflags(vcpu);
        case CPU_REG_IP: return VCPU_grip(vcpu);

        default:
            printf("VCPU_reg_get: Invalid guest CPU register is given (sel:%u)!\n", sel);
            HALT();
            return 0; // should never hit
    }
}

/*
 * Set a guest register
 */
void VCPU_reg_set(VCPU *vcpu, struct regs* r, enum CPU_Reg_Sel sel, ulong_t val)
{
    switch (sel)
    {
#ifdef __AMD64__
        case CPU_REG_AX: r->rax = val; break;
        case CPU_REG_CX: r->rcx = val; break;
        case CPU_REG_DX: r->rdx = val; break;
        case CPU_REG_BX: r->rbx = val; break;
        /* CPU_REG_SP is managed by VCPU_grsp_set() */
        case CPU_REG_BP: r->rbp = val; break;
        case CPU_REG_SI: r->rsi = val; break;
        case CPU_REG_DI: r->rdi = val; break;
        case CPU_REG_R8: r->r8 = val; break;
        case CPU_REG_R9: r->r9 = val; break;
        case CPU_REG_R10: r->r10 = val; break;
        case CPU_REG_R11: r->r11 = val; break;
        case CPU_REG_R12: r->r12 = val; break;
        case CPU_REG_R13: r->r13 = val; break;
        case CPU_REG_R14: r->r14 = val; break;
        case CPU_REG_R15: r->r15 = val; break;
#elif defined(__I386__)
        case CPU_REG_AX: r->eax = val; break;
        case CPU_REG_CX: r->ecx = val; break;
        case CPU_REG_DX: r->edx = val; break;
        case CPU_REG_BX: r->ebx = val; break;
        /* CPU_REG_SP is managed by VCPU_grsp_set() */
        case CPU_REG_BP: r->ebp = val; break;
        case CPU_REG_SI: r->esi = val; break;
        case CPU_REG_DI: r->edi = val; break;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

        case CPU_REG_SP: 
            VCPU_grsp_set(vcpu, val);
            break;
        case CPU_REG_FLAGS:
            VCPU_grflags_set(vcpu, val);
            break;
        case CPU_REG_IP:
            VCPU_grip_set(vcpu, val);
            break;

        default:
            printf("VCPU_reg_set: Invalid guest CPU register is given (sel:%u)!\n", sel);
            HALT();
    }
}

