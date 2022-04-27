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

// EMHF DMA protection component implementation
// x86 backends
// author: amit vasudevan (amitvasudevan@acm.org)

#include <xmhf.h> 

//return size (in bytes) of the memory buffer required for
//DMA protection for a given physical memory limit
u32 xmhf_dmaprot_arch_getbuffersize(u64 physical_memory_limit){
	u32 cpu_vendor = get_cpu_vendor_or_die();	//determine CPU vendor
	HALT_ON_ERRORCOND( physical_memory_limit <= ADDR_4GB ); 	//we only support 4GB physical memory currently
	
	if(cpu_vendor == CPU_VENDOR_AMD){
		return ((physical_memory_limit / PAGE_SIZE_4K) / 8); //each page takes up 1-bit with AMD DEV
	}else{	//CPU_VENDOR_INTEL
		return (PAGE_SIZE_4K + (PAGE_SIZE_4K * PAE_PTRS_PER_PDPT) + (PAGE_SIZE_4K * PAE_PTRS_PER_PDPT * PAE_PTRS_PER_PDT) + PAGE_SIZE_4K +	(PAGE_SIZE_4K * PCI_BUS_MAX));	//4-level PML4 page tables + 4KB root entry table + 4K context entry table per PCI bus
	}
}


//"early" DMA protection initialization to setup minimal
//structures to protect a range of physical memory
//return 1 on success 0 on failure
u32 xmhf_dmaprot_arch_earlyinitialize(u64 protectedbuffer_paddr, u32 protectedbuffer_vaddr, u32 protectedbuffer_size, u64 memregionbase_paddr, u32 memregion_size){
	u32 cpu_vendor = get_cpu_vendor_or_die();	//determine CPU vendor
	
	
	if(cpu_vendor == CPU_VENDOR_AMD){
	  return xmhf_dmaprot_arch_x86svm_earlyinitialize(protectedbuffer_paddr, protectedbuffer_vaddr, protectedbuffer_size, memregionbase_paddr,	memregion_size);
	}
	else{	//CPU_VENDOR_INTEL
	  return xmhf_dmaprot_arch_x86vmx_earlyinitialize(protectedbuffer_paddr, protectedbuffer_vaddr, protectedbuffer_size, memregionbase_paddr, 	memregion_size);
	}
}

//"normal" DMA protection initialization to setup required
//structures for DMA protection
//return 1 on success 0 on failure
u32 xmhf_dmaprot_arch_initialize(u64 protectedbuffer_paddr,
	u32 protectedbuffer_vaddr, u32 protectedbuffer_size){
	u32 cpu_vendor = get_cpu_vendor_or_die();	//determine CPU vendor

	if(cpu_vendor == CPU_VENDOR_AMD){
	  return xmhf_dmaprot_arch_x86svm_initialize(protectedbuffer_paddr,	protectedbuffer_vaddr, protectedbuffer_size);
	}else{	//CPU_VENDOR_INTEL
	  return 1; //we use Vtd PMRs to protect the SL + runtime during SL launch
	}
		
}


//DMA protect a given region of memory, start_paddr is
//assumed to be page aligned physical memory address
void xmhf_dmaprot_arch_protect(u32 start_paddr, u32 size){
	u32 cpu_vendor = get_cpu_vendor_or_die();	//determine CPU vendor

	if(cpu_vendor == CPU_VENDOR_AMD){
	  return xmhf_dmaprot_arch_x86svm_protect(start_paddr, size);
	}else{	//CPU_VENDOR_INTEL
	  return; //we use Vtd PMRs to protect the SL + runtime during SL launch
	} 
}
