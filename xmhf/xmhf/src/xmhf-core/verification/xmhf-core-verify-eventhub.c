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

//----------------------------------------------------------------------
// xmhf-core-verify-eventhub.c
// XMHF core eventhub verification driver
// author: amit vasudevan (amitvasudevan@acm.org)
//----------------------------------------------------------------------
#include <xmhf.h>

#define V_HYPERCALL		0xDEADBEEF

u32 xmhf_verify_cpu_vendor;

VCPU vcpu;
struct regs r;
struct _svm_vmcbfields _xvmcb;

//globals referenced by this module
RPB *rpb; 	//runtime parameter block pointer

//actual definitions
RPB _xrpb;	

void main() {
		//setup RPB pointer and required runtime parameter block values
		rpb = (RPB *)&_xrpb;
		rpb->XtVmmE820NumEntries = 1; 									//number of E820 entries
		rpb->XtVmmRuntimePhysBase = 0xC0000000;							//runtime physical base address
		rpb->XtVmmRuntimeSize = 0x8800000;								//128 MB + 8MB (HPTs) runtime size
		rpb->XtGuestOSBootModuleBase = 0x20000;							//guest OS boot module base address
		rpb->XtGuestOSBootModuleSize = 512;								//guest OS boot module size

		//setup bare minimum vcpu
		vcpu.isbsp = 1;													//assume BSP
		vcpu.id = 0;													//give a LAPIC id
		vcpu.esp = 0xC6000000;											//give a stack

		//globals
		g_midtable_numentries=1;										//number of CPU table entries
		g_svm_lapic_base = 0xFEE00000;									//CPU LAPIC physical addresses
		g_vmx_lapic_base = 0xFEE00000;

#if defined (__XMHF_TARGET_ARCH_X86_VMX__)
		xmhf_verify_cpu_vendor = CPU_VENDOR_INTEL;
		vcpu.cpu_vendor = CPU_VENDOR_INTEL;								
#else
		xmhf_verify_cpu_vendor = CPU_VENDOR_AMD;
		vcpu.cpu_vendor = CPU_VENDOR_AMD;
#endif		
		

#if defined (__XMHF_TARGET_ARCH_X86_VMX__)
		//Intel specific fields
		vcpu.vmx_vmcs_vaddr = 0xC7000000;								//VMCS address
		vcpu.vmx_vaddr_ept_pml4_table = 0xC7F00000;						//EPT PML4 table 		
		vcpu.vmx_guest_unrestricted = 1;								//VMX unrestricted guest support
		vcpu.vmx_vaddr_ept_p_tables = 0xC8000000;						//EPT page tables
#else
		//AMD specific fields
		vcpu.npt_vaddr_ptr = 0xC7F00000;								//NPT PDPT page
		vcpu.npt_vaddr_pts = 0xC8000000;								//NPT page tables
		vcpu.vmcb_vaddr_ptr = &_xvmcb;									//set vcpu VMCB virtual address to something meaningful
#endif
		

#if defined (__XMHF_TARGET_ARCH_X86_VMX__)
		//VMX propMED values after init()
		vcpu.vmcs.control_VMX_seccpu_based |= (1 << 1); //enable EPT
		vcpu.vmcs.control_EPT_pointer_high = 0;
		vcpu.vmcs.control_EPT_pointer_full = hva2spa((void*)vcpu.vmx_vaddr_ept_pml4_table) | 0x1E; //page walk of 4 and WB memory
#else
		//SVM propMED values after init()
		_xvmcb.n_cr3 = hva2spa((void*)vcpu.npt_vaddr_ptr);
		_xvmcb.np_enable |= 1ULL;
#endif

		//setup CPU general purpose register state (non-deterministic)
		r.eax = r.ebx = r.ecx= r.edx = r.esi = r.edi = r.ebp = r.esp = nondet_u32(); 

#if defined (__XMHF_TARGET_ARCH_X86_VMX__)
		//VMX non-deterministic state
		{
			vcpu.vmcs.info_vminstr_error = nondet_u32();
			vcpu.vmcs.info_vmexit_reason= nondet_u32();
			vcpu.vmcs.info_vmexit_interrupt_information=nondet_u32();
			vcpu.vmcs.info_vmexit_interrupt_error_code=nondet_u32();
			vcpu.vmcs.info_IDT_vectoring_information=nondet_u32();
			vcpu.vmcs.info_IDT_vectoring_error_code=nondet_u32();
			vcpu.vmcs.info_vmexit_instruction_length=nondet_u32();
			vcpu.vmcs.info_vmx_instruction_information=nondet_u32();
			vcpu.vmcs.info_exit_qualification=nondet_u64();
			vcpu.vmcs.info_IO_RCX=nondet_u64();
			vcpu.vmcs.info_IO_RSI=nondet_u64();
			vcpu.vmcs.info_IO_RDI=nondet_u64();
			vcpu.vmcs.info_IO_RIP=nondet_u64();
			vcpu.vmcs.info_guest_linear_address=nondet_u64();		
			vcpu.vmcs.guest_paddr_full=nondet_u64();

			vcpu.vmcs.guest_CR0=nondet_u64();
			vcpu.vmcs.guest_CR3=nondet_u64();
			vcpu.vmcs.guest_CR4=nondet_u64();
			vcpu.vmcs.guest_ES_base=nondet_u64();
			vcpu.vmcs.guest_CS_base=nondet_u64(); 
			vcpu.vmcs.guest_SS_base=nondet_u64();
			vcpu.vmcs.guest_DS_base=nondet_u64();
			vcpu.vmcs.guest_FS_base=nondet_u64();
			vcpu.vmcs.guest_GS_base=nondet_u64();
			vcpu.vmcs.guest_LDTR_base=nondet_u64();
			vcpu.vmcs.guest_TR_base=nondet_u64();
			vcpu.vmcs.guest_GDTR_base=nondet_u64();
			vcpu.vmcs.guest_IDTR_base=nondet_u64();
			vcpu.vmcs.guest_DR7=nondet_u64();
			vcpu.vmcs.guest_RSP=nondet_u64(); 
			vcpu.vmcs.guest_RIP=nondet_u64(); 
			vcpu.vmcs.guest_RFLAGS=nondet_u64(); 
			vcpu.vmcs.guest_pending_debug_x=nondet_u64();
			vcpu.vmcs.guest_SYSENTER_ESP=nondet_u64();
			vcpu.vmcs.guest_SYSENTER_EIP=nondet_u64();

			vcpu.vmcs.guest_ES_limit=nondet_u32();
			vcpu.vmcs.guest_CS_limit=nondet_u32();
			vcpu.vmcs.guest_SS_limit=nondet_u32();
			vcpu.vmcs.guest_DS_limit=nondet_u32();
			vcpu.vmcs.guest_FS_limit=nondet_u32();
			vcpu.vmcs.guest_GS_limit=nondet_u32();
			vcpu.vmcs.guest_LDTR_limit=nondet_u32(); 
			vcpu.vmcs.guest_TR_limit=nondet_u32();
			vcpu.vmcs.guest_GDTR_limit=nondet_u32();
			vcpu.vmcs.guest_IDTR_limit=nondet_u32();
			vcpu.vmcs.guest_ES_access_rights=nondet_u32(); 
			vcpu.vmcs.guest_CS_access_rights=nondet_u32();
			vcpu.vmcs.guest_SS_access_rights=nondet_u32();
			vcpu.vmcs.guest_DS_access_rights=nondet_u32();
			vcpu.vmcs.guest_FS_access_rights=nondet_u32();
			vcpu.vmcs.guest_GS_access_rights=nondet_u32();
			vcpu.vmcs.guest_LDTR_access_rights=nondet_u32();
			vcpu.vmcs.guest_TR_access_rights=nondet_u32();
			vcpu.vmcs.guest_interruptibility=nondet_u32(); 
			vcpu.vmcs.guest_activity_state=nondet_u32(); 
			vcpu.vmcs.guest_SMBASE=nondet_u32();	
			vcpu.vmcs.guest_SYSENTER_CS=nondet_u32(); 

			vcpu.vmcs.guest_ES_selector=nondet_u16();
			vcpu.vmcs.guest_CS_selector=nondet_u16();
			vcpu.vmcs.guest_SS_selector=nondet_u16();
			vcpu.vmcs.guest_DS_selector=nondet_u16();
			vcpu.vmcs.guest_FS_selector=nondet_u16();
			vcpu.vmcs.guest_GS_selector=nondet_u16();
			vcpu.vmcs.guest_LDTR_selector=nondet_u16();
			vcpu.vmcs.guest_TR_selector=nondet_u16();
		}

#else
		//SVM non-deterministic state
		{
			_xvmcb.exitcode = (u64)nondet_u64();
			_xvmcb.exitinfo1 = (u64)nondet_u64();
			_xvmcb.exitinfo2 = (u64)nondet_u64();

			_xvmcb.es.attrib = (u16)nondet_u16();
			_xvmcb.es.base = (u64)nondet_u64();
			_xvmcb.es.limit = (u32)nondet_u32();
			_xvmcb.es.selector = (u16)nondet_u16();

			_xvmcb.cs.attrib = (u16)nondet_u16();
			_xvmcb.cs.base = (u64)nondet_u64();
			_xvmcb.cs.limit = (u32)nondet_u32();
			_xvmcb.cs.selector = (u16)nondet_u16();

			_xvmcb.ss.attrib = (u16)nondet_u16();
			_xvmcb.ss.base = (u64)nondet_u64();
			_xvmcb.ss.limit = (u32)nondet_u32();
			_xvmcb.ss.selector = (u16)nondet_u16();

			_xvmcb.ds.attrib = (u16)nondet_u16();
			_xvmcb.ds.base = (u64)nondet_u64();
			_xvmcb.ds.limit = (u32)nondet_u32();
			_xvmcb.ds.selector = (u16)nondet_u16();

			_xvmcb.fs.attrib = (u16)nondet_u16();
			_xvmcb.fs.base = (u64)nondet_u64();
			_xvmcb.fs.limit = (u32)nondet_u32();
			_xvmcb.fs.selector = (u16)nondet_u16();

			_xvmcb.gs.attrib = (u16)nondet_u16();
			_xvmcb.gs.base = (u64)nondet_u64();
			_xvmcb.gs.limit = (u32)nondet_u32();
			_xvmcb.gs.selector = (u16)nondet_u16();

			_xvmcb.gdtr.attrib = (u16)nondet_u16();
			_xvmcb.gdtr.base = (u64)nondet_u64();
			_xvmcb.gdtr.limit = (u32)nondet_u32();
			_xvmcb.gdtr.selector = (u16)nondet_u16();

			_xvmcb.ldtr.attrib = (u16)nondet_u16();
			_xvmcb.ldtr.base = (u64)nondet_u64();
			_xvmcb.ldtr.limit = (u32)nondet_u32();
			_xvmcb.ldtr.selector = (u16)nondet_u16();
			
			_xvmcb.idtr.attrib = (u16)nondet_u16();
			_xvmcb.idtr.base = (u64)nondet_u64();
			_xvmcb.idtr.limit = (u32)nondet_u32();
			_xvmcb.idtr.selector = (u16)nondet_u16();

			_xvmcb.tr.attrib = (u16)nondet_u16();
			_xvmcb.tr.base = (u64)nondet_u64();
			_xvmcb.tr.limit = (u32)nondet_u32();
			_xvmcb.tr.selector = (u16)nondet_u16();
			
			_xvmcb.cr4 = (u64)nondet_u64();
			_xvmcb.cr3 = (u64)nondet_u64();
			_xvmcb.cr0 = (u64)nondet_u64();
			_xvmcb.dr7 = (u64)nondet_u64();
			_xvmcb.dr6 = (u64)nondet_u64();
			_xvmcb.rflags = (u64)nondet_u64();
			_xvmcb.rip = (u64)nondet_u64();
			_xvmcb.rsp = (u64)nondet_u64();
			_xvmcb.rax = (u64)nondet_u64();
			_xvmcb.cr2 = (u64)nondet_u64();
			_xvmcb.g_pat = (u64)nondet_u64();
			_xvmcb.efer = (u64)nondet_u64();                   

		}	
#endif

#if defined (__XMHF_TARGET_ARCH_X86_VMX__)
			xmhf_parteventhub_arch_x86vmx_intercept_handler(&vcpu, &r);
#else
			xmhf_parteventhub_arch_x86svm_intercept_handler(&vcpu, &r);
#endif
		
		assert(1);
}
//----------------------------------------------------------------------
