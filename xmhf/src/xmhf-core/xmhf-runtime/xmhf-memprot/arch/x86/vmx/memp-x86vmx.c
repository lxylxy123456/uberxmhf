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

// EMHF memory protection component
// intel VMX arch. backend implementation
// author: amit vasudevan (amitvasudevan@acm.org)

#include <xmhf.h>

//----------------------------------------------------------------------
// Structure that captures fixed MTRR properties
struct _fixed_mtrr_prop_t {
	u32 msr;	/* MSR register address (ECX in RDMSR / WRMSR) */
	u32 start;	/* Start address */
	u32 step;	/* Size of each range */
	u32 end;	/* End address (start + 8 * step == end) */
} fixed_mtrr_prop[NUM_FIXED_MTRRS] = {
	{IA32_MTRR_FIX64K_00000, 0x00000000, 0x00010000, 0x00080000},
	{IA32_MTRR_FIX16K_80000, 0x00080000, 0x00004000, 0x000A0000},
	{IA32_MTRR_FIX16K_A0000, 0x000A0000, 0x00004000, 0x000C0000},
	{IA32_MTRR_FIX4K_C0000, 0x000C0000, 0x00001000, 0x000C8000},
	{IA32_MTRR_FIX4K_C8000, 0x000C8000, 0x00001000, 0x000D0000},
	{IA32_MTRR_FIX4K_D0000, 0x000D0000, 0x00001000, 0x000D8000},
	{IA32_MTRR_FIX4K_D8000, 0x000D8000, 0x00001000, 0x000E0000},
	{IA32_MTRR_FIX4K_E0000, 0x000E0000, 0x00001000, 0x000E8000},
	{IA32_MTRR_FIX4K_E8000, 0x000E8000, 0x00001000, 0x000F0000},
	{IA32_MTRR_FIX4K_F0000, 0x000F0000, 0x00001000, 0x000F8000},
	{IA32_MTRR_FIX4K_F8000, 0x000F8000, 0x00001000, 0x00100000},
};

//----------------------------------------------------------------------
// local (static) support function forward declarations
static void _vmx_gathermemorytypes(VCPU *vcpu);
static u32 _vmx_getmemorytypeforphysicalpage(VCPU *vcpu, u64 pagebaseaddr);
static void _vmx_setupEPT(VCPU *vcpu);

//======================================================================
// global interfaces (functions) exported by this component

// initialize memory protection structures for a given core (vcpu)
void xmhf_memprot_arch_x86vmx_initialize(VCPU *vcpu){
	HALT_ON_ERRORCOND(vcpu->cpu_vendor == CPU_VENDOR_INTEL);

	_vmx_gathermemorytypes(vcpu);
#ifndef __XMHF_VERIFICATION__
	_vmx_setupEPT(vcpu);
#endif

	vcpu->vmcs.control_VMX_seccpu_based |= (1 << 1); //enable EPT
	vcpu->vmcs.control_VMX_seccpu_based |= (1 << 5); //enable VPID
	vcpu->vmcs.control_vpid = 1; //VPID=0 is reserved for hypervisor
	vcpu->vmcs.control_EPT_pointer = hva2spa((void*)vcpu->vmx_vaddr_ept_pml4_table) | 0x1E; //page walk of 4 and WB memory
	vcpu->vmcs.control_VMX_cpu_based &= ~(1 << 15); //disable CR3 load exiting
	vcpu->vmcs.control_VMX_cpu_based &= ~(1 << 16); //disable CR3 store exiting
}


//----------------------------------------------------------------------
// local (static) support functions follow


//---gather memory types for system physical memory------------------------------
static void _vmx_gathermemorytypes(VCPU *vcpu){
 	u32 eax, ebx, ecx, edx;
	u32 num_vmtrrs=0;	//number of variable length MTRRs supported by the CPU

	//0. sanity check
  	//check MTRR support
  	eax=0x00000001;
  	ecx=0x00000000;
	#ifndef __XMHF_VERIFICATION__
  	asm volatile ("cpuid\r\n"
            :"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            :"a"(eax), "c" (ecx));
  	#endif

  	if( !(edx & (u32)(1 << 12)) ){
  		printf("\nCPU(0x%02x): CPU does not support MTRRs!", vcpu->id);
  		HALT();
  	}

  	//check MTRR caps
  	rdmsr(IA32_MTRRCAP, &eax, &edx);
	num_vmtrrs = (u8)eax;
	vcpu->vmx_guestmtrrmsrs.var_count = num_vmtrrs;
  	printf("\nIA32_MTRRCAP: VCNT=%u, FIX=%u, WC=%u, SMRR=%u",
  		num_vmtrrs, ((eax & (1 << 8)) >> 8),  ((eax & (1 << 10)) >> 10),
  			((eax & (1 << 11)) >> 11));
	//sanity check that fixed MTRRs are supported
  	vcpu->vmx_ept_fixmtrr_enable = ((eax & (1 << 8)) >> 8);
  	HALT_ON_ERRORCOND( vcpu->vmx_ept_fixmtrr_enable );
  	//ensure number of variable MTRRs are within the maximum supported
  	HALT_ON_ERRORCOND( (num_vmtrrs <= MAX_VARIABLE_MTRR_PAIRS) );

	// Read MTRR default type register
	rdmsr(IA32_MTRR_DEF_TYPE, &eax, &edx);
	HALT_ON_ERRORCOND(edx == 0);	/* All bits are reserved */
	vcpu->vmx_guestmtrrmsrs.def_type = eax;
	vcpu->vmx_ept_defaulttype = (eax & 0xFFU);
	// Sanity check that Fixed-range MTRRs are enabled
	HALT_ON_ERRORCOND(eax & (1 << 10));
	// Sanity check that MTRRs are enabled
	HALT_ON_ERRORCOND(eax & (1 << 11));

	// Fill guest MTRR shadow MSRs
	for (u32 i = 0; i < NUM_FIXED_MTRRS; i++) {
		vcpu->vmx_guestmtrrmsrs.fix_mtrrs[i] = rdmsr64(fixed_mtrr_prop[i].msr);
	}
	for (u32 i = 0; i < num_vmtrrs; i++) {
		vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].base =
			rdmsr64(IA32_MTRR_PHYSBASE0 + 2 * i);
		vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].mask =
			rdmsr64(IA32_MTRR_PHYSMASK0 + 2 * i);
	}
}

//---get memory type for a given physical page address--------------------------
//
//11.11.4.1 MTRR Precedences
//  0. if MTRRs are not enabled --> MTRR_TYPE_UC
//  if enabled then
     //if physaddr < 1MB use fixed MTRR ranges return type
     //else if within a valid variable range MTRR then
        //if a single match, return type
        //if two or more and one is UC, return UC
        //if two or more and WB and WT, return WT
        //else invalid combination
     //else
       // return default memory type
//
static u32 _vmx_getmemorytypeforphysicalpage(VCPU *vcpu, u64 pagebaseaddr){
	u32 prev_type = MTRR_TYPE_RESV;
	/* If fixed MTRRs are enabled, and addr < 1M, use them */
	if (pagebaseaddr < 0x100000ULL && vcpu->vmx_ept_fixmtrr_enable) {
		for (u32 i = 0; i < NUM_FIXED_MTRRS; i++) {
			struct _fixed_mtrr_prop_t *prop = &fixed_mtrr_prop[i];
			if (pagebaseaddr < prop->end) {
				u32 index = (prop->start - pagebaseaddr) / prop->step;
				u64 msrval = vcpu->vmx_guestmtrrmsrs.fix_mtrrs[i];
				return (u8) (msrval >> (index * 8));
			}
		}
		/*
		 * Should be impossible because the last entry in fixed_mtrr_prop is 1M.
		 */
		HALT_ON_ERRORCOND(0 && "Unknown fixed MTRR");
	}
	/* Compute variable MTRRs */
	for (u32 i = 0; i < vcpu->vmx_guestmtrrmsrs.var_count; i++) {
		u64 base = vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].base;
		u64 mask = vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].mask;
		u32 cur_type = base & 0xFFU;
		/* Check valid bit */
		if (!(mask & (1ULL << 11))) {
			continue;
		}
		/* Clear lower bits, test whether address in range */
		base &= ~0xFFFULL;
		mask &= ~0xFFFULL;
		if ((pagebaseaddr & mask) != (base & mask)) {
			continue;
		}
		/* Check for conflict resolutions: UC + * = UC; WB + WT = WT */
		if (prev_type == MTRR_TYPE_RESV || prev_type == cur_type) {
			prev_type = cur_type;
		} else if (prev_type == MTRR_TYPE_UC || cur_type == MTRR_TYPE_UC) {
			prev_type = MTRR_TYPE_UC;
		} else if (prev_type == MTRR_TYPE_WB && cur_type == MTRR_TYPE_WT) {
			prev_type = MTRR_TYPE_WT;
		} else if (prev_type == MTRR_TYPE_WT && cur_type == MTRR_TYPE_WB) {
			prev_type = MTRR_TYPE_WT;
		} else {
			printf("\nConflicting MTRR types (%u, %u), HALT!", prev_type, cur_type);
			HALT();
		}
	}
	/* If not covered by any MTRR, use default type */
	if (prev_type == MTRR_TYPE_RESV) {
		prev_type = vcpu->vmx_ept_defaulttype;
	}
	return prev_type;
}


//---setup EPT for VMX----------------------------------------------------------
static void _vmx_setupEPT(VCPU *vcpu){
	//step-1: tie in EPT PML4 structures
	u64 *pml4_entry = (u64 *)vcpu->vmx_vaddr_ept_pml4_table;
	u64 *pdp_entry = (u64 *)vcpu->vmx_vaddr_ept_pdp_table;
	u64 *pd_entry = (u64 *)vcpu->vmx_vaddr_ept_pd_tables;
	u64 *p_entry = (u64 *)vcpu->vmx_vaddr_ept_p_tables;
	u64 pdp_table = hva2spa((void*)vcpu->vmx_vaddr_ept_pdp_table);
	u64 pd_table = hva2spa((void*)vcpu->vmx_vaddr_ept_pd_tables);
	u64 p_table = hva2spa((void*)vcpu->vmx_vaddr_ept_p_tables);
	// TODO: for amd64, likely can use 1G / 2M pages.
	// If so, also need to change _vmx_getmemorytypeforphysicalpage()

	for (u64 paddr = 0; paddr < MAX_PHYS_ADDR; paddr += PA_PAGE_SIZE_4K) {
		if (PA_PAGE_ALIGNED_512G(paddr)) {
			/* Create PML4E */
			u64 i = paddr / PA_PAGE_SIZE_512G;
			pml4_entry[i] = (pdp_table + i * PA_PAGE_SIZE_4K) | 0x7;
		}
		if (PA_PAGE_ALIGNED_1G(paddr)) {
			/* Create PDPE */
			u64 i = paddr / PA_PAGE_SIZE_1G;
			pdp_entry[i] = (pd_table + i * PA_PAGE_SIZE_4K) | 0x7;
		}
		if (PA_PAGE_ALIGNED_2M(paddr)) {
			/* Create PDE */
			u64 i = paddr / PA_PAGE_SIZE_2M;
			pd_entry[i] = (p_table + i * PA_PAGE_SIZE_4K) | 0x7;
		}
		/* PA_PAGE_ALIGNED_4K */
		{
			/* Create PE */
			u64 i = paddr / PA_PAGE_SIZE_4K;
			u64 memorytype = _vmx_getmemorytypeforphysicalpage(vcpu, paddr);
			u64 lower;
			/*
			 * For memorytype equal to 0 (UC), 1 (WC), 4 (WT), 5 (WP), 6 (WB),
			 * MTRR memory type and EPT memory type are the same encoding.
			 * Currently other encodings are reserved.
			 */
			HALT_ON_ERRORCOND(memorytype == 0 || memorytype == 1 ||
								memorytype == 4 || memorytype == 5 ||
								memorytype == 6);
			if ((paddr >= (rpb->XtVmmRuntimePhysBase - PA_PAGE_SIZE_2M)) &&
				(paddr < (rpb->XtVmmRuntimePhysBase + rpb->XtVmmRuntimeSize))) {
				lower = 0x0;	/* not present */
			} else {
				lower = 0x7;	/* present */
			}
			p_entry[i] = paddr | (memorytype << 3) | lower;
		}
	}
}


//flush hardware page table mappings (TLB)
void xmhf_memprot_arch_x86vmx_flushmappings(VCPU *vcpu){
  __vmx_invept(VMX_INVEPT_SINGLECONTEXT,
          (u64)vcpu->vmcs.control_EPT_pointer);
}

//flush hardware page table mappings (TLB)
void xmhf_memprot_arch_x86vmx_flushmappings_localtlb(VCPU *vcpu){
	(void)vcpu;
  __vmx_invept(VMX_INVEPT_GLOBAL,
          (u64)0);
}

//set protection for a given physical memory address
void xmhf_memprot_arch_x86vmx_setprot(VCPU *vcpu, u64 gpa, u32 prottype){
  u32 pfn;
  u64 *pt;
  u32 flags =0;

#ifdef __XMHF_VERIFICATION_DRIVEASSERTS__
   	assert ( (vcpu != NULL) );
	assert ( ( (gpa < rpb->XtVmmRuntimePhysBase) ||
							 (gpa >= (rpb->XtVmmRuntimePhysBase + rpb->XtVmmRuntimeSize))
						   ) );
	assert ( ( (prottype > 0)	&&
	                         (prottype <= MEMP_PROT_MAXVALUE)
	                       ) );
	assert (
	 (prottype == MEMP_PROT_NOTPRESENT) ||
	 ((prottype & MEMP_PROT_PRESENT) && (prottype & MEMP_PROT_READONLY) && (prottype & MEMP_PROT_EXECUTE)) ||
	 ((prottype & MEMP_PROT_PRESENT) && (prottype & MEMP_PROT_READWRITE) && (prottype & MEMP_PROT_EXECUTE)) ||
	 ((prottype & MEMP_PROT_PRESENT) && (prottype & MEMP_PROT_READONLY) && (prottype & MEMP_PROT_NOEXECUTE)) ||
	 ((prottype & MEMP_PROT_PRESENT) && (prottype & MEMP_PROT_READWRITE) && (prottype & MEMP_PROT_NOEXECUTE))
	);
#endif

  pfn = (u32)gpa / PAGE_SIZE_4K;	//grab page frame number
  pt = (u64 *)vcpu->vmx_vaddr_ept_p_tables;

  //default is not-present, read-only, no-execute
  pt[pfn] &= ~(u64)7; //clear all previous flags

  //map high level protection type to EPT protection bits
  if(prottype & MEMP_PROT_PRESENT){
	flags=1;	//present is defined by the read bit in EPT

	if(prottype & MEMP_PROT_READWRITE)
		flags |= 0x2;

	if(prottype & MEMP_PROT_EXECUTE)
		flags |= 0x4;
  }

  //set new flags
  pt[pfn] |= flags;
}


//get protection for a given physical memory address
u32 xmhf_memprot_arch_x86vmx_getprot(VCPU *vcpu, u64 gpa){
  u32 pfn = (u32)gpa / PAGE_SIZE_4K;	//grab page frame number
  u64 *pt = (u64 *)vcpu->vmx_vaddr_ept_p_tables;
  u64 entry = pt[pfn];
  u32 prottype;

  if(! (entry & 0x1) ){
	prottype = MEMP_PROT_NOTPRESENT;
	return prottype;
  }

  prottype = MEMP_PROT_PRESENT;

  if( entry & 0x2 )
	prottype |= MEMP_PROT_READWRITE;
  else
	prottype |= MEMP_PROT_READONLY;

  if( entry & 0x4 )
	prottype |= MEMP_PROT_EXECUTE;
  else
	prottype |= MEMP_PROT_NOEXECUTE;

  return prottype;
}

u64 xmhf_memprot_arch_x86vmx_get_EPTP(VCPU *vcpu)
{
  HALT_ON_ERRORCOND(vcpu->cpu_vendor == CPU_VENDOR_INTEL);
  return vcpu->vmcs.control_EPT_pointer;
}
void xmhf_memprot_arch_x86vmx_set_EPTP(VCPU *vcpu, u64 eptp)
{
  HALT_ON_ERRORCOND(vcpu->cpu_vendor == CPU_VENDOR_INTEL);
  vcpu->vmcs.control_EPT_pointer = eptp;
}
