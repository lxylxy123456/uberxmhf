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
 * EMHF base platform component interface
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf.h>

//get CPU vendor
u32 xmhf_baseplatform_getcpuvendor(void){
	return xmhf_baseplatform_arch_getcpuvendor();
}

//initialize basic platform elements
void xmhf_baseplatform_initialize(void){
	xmhf_baseplatform_arch_initialize();	
}

//initialize CPU state
void xmhf_baseplatform_cpuinitialize(void){
	xmhf_baseplatform_arch_cpuinitialize();
}

extern RPB *rpb;
// extern GRUBE820 g_e820map[];

// Traverse the E820 map and return the lower and upper used system physical address (i.e., used by main memory and MMIO).
// [NOTE] <machine_high_spa> must be u64 even on 32-bit machines, because it could be 4G, and hence overflow u32.
// [TODO][Issue 85] Move this function to a better place
bool xmhf_baseplatform_x86_e820_paddr_range(spa_t* machine_low_spa, u64* machine_high_spa)
{
	u32 e820_last_idx = 0;
	spa_t last_e820_entry_base = INVALID_SPADDR;
	size_t last_e820_entry_len = 0;

	// Sanity checks
	if(!machine_low_spa || !machine_high_spa)
		return false;

	if(!rpb)
		return false;

	if(!rpb->XtVmmE820NumEntries)
		return false;

	// Calc <machine_low_spa> and <machine_high_spa>
	e820_last_idx = rpb->XtVmmE820NumEntries - 1;

	*machine_low_spa = UINT32sToSPADDR(g_e820map[0].baseaddr_high, g_e820map[0].baseaddr_low);
	last_e820_entry_base = UINT32sToSPADDR(g_e820map[e820_last_idx].baseaddr_high, g_e820map[e820_last_idx].baseaddr_low);
	last_e820_entry_len = UINT32sToSIZE(g_e820map[e820_last_idx].baseaddr_high, g_e820map[e820_last_idx].baseaddr_low);
	*machine_high_spa = (spa_t)(last_e820_entry_base + last_e820_entry_len);
				
	return true;
}