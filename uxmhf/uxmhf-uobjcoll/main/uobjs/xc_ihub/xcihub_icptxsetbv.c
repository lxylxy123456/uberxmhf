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

#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/xmhf.h>
#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/geec.h>
// #include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/xmhf-debug.h>

#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/xc.h>
#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/xc_ihub.h>
#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/uapi_gcpustate.h>
#include <uberspark/uobjcoll/platform/pc/uxmhf/main/include/uapi_hcpustate.h>

/*
 * xcihub_icptxsetbv -- rich guest XSETBV instruction emulation
 *
 * author: amit vasudevan (amitvasudevan@acm.org)
 */
void xcihub_icptxsetbv(uint32_t cpuid){
	slab_params_t spl;
	xmhf_uapi_gcpustate_vmrw_params_t *gcpustate_vmrwp = (xmhf_uapi_gcpustate_vmrw_params_t *)spl.in_out_params;
	xmhf_uapi_gcpustate_gprs_params_t *gcpustate_gprs = (xmhf_uapi_gcpustate_gprs_params_t *)spl.in_out_params;
	uint32_t guest_rip;
	uint32_t info_vmexit_instruction_length;
	x86regs_t r;
	uint64_t xcr_value;

	//_XDPRINTF_("%s[%u]: CRX access\n", __func__, cpuid);

	uberspark_uobjrtl_crt__memset(&spl, 0, sizeof(spl));
	spl.cpuid = cpuid;
	spl.src_slabid = XMHFGEEC_SLAB_XC_IHUB;
	spl.dst_slabid = XMHFGEEC_SLAB_UAPI_GCPUSTATE;

	//read GPRs
	spl.dst_uapifn = XMHF_HIC_UAPI_CPUSTATE_GUESTGPRSREAD;
	XMHF_SLAB_CALLNEW(&spl);
	memcpy(&r, &gcpustate_gprs->gprs, sizeof(x86regs_t));

   	xcr_value = ((uint64_t)r.edx << 32) + (uint64_t)r.eax;

   	if(r.ecx != XCR_XFEATURE_ENABLED_MASK){
   		_XDPRINTF_("%s[%u]: unhandled XCR register %u", __func__, cpuid, r.ecx);
		CASM_FUNCCALL(uberspark_uobjrtl_hw__generic_x86_32_intel__hlt, CASM_NOPARAM);
	}

	//XXX: TODO: check for invalid states and inject GP accordingly
	_XDPRINTF_("%s[%u]: xcr_value=%llx", __func__, cpuid, xcr_value);

	//set XCR with supplied value
	CASM_FUNCCALL(xsetbv, XCR_XFEATURE_ENABLED_MASK, r.eax, r.edx);

	//skip over XSETBV instruction by adjusting RIP
	spl.dst_uapifn = XMHF_HIC_UAPI_CPUSTATE_VMREAD;
	gcpustate_vmrwp->encoding = VMCS_INFO_VMEXIT_INSTRUCTION_LENGTH;
	XMHF_SLAB_CALLNEW(&spl);
	info_vmexit_instruction_length = gcpustate_vmrwp->value;

	gcpustate_vmrwp->encoding = VMCS_GUEST_RIP;
	XMHF_SLAB_CALLNEW(&spl);
	guest_rip = gcpustate_vmrwp->value;
	guest_rip+=info_vmexit_instruction_length;

	spl.dst_uapifn = XMHF_HIC_UAPI_CPUSTATE_VMWRITE;
	gcpustate_vmrwp->encoding = VMCS_GUEST_RIP;
	gcpustate_vmrwp->value = guest_rip;
	XMHF_SLAB_CALLNEW(&spl);

	//write interruptibility = 0
	gcpustate_vmrwp->encoding = VMCS_GUEST_INTERRUPTIBILITY;
	gcpustate_vmrwp->value = 0;
	XMHF_SLAB_CALLNEW(&spl);

	//_XDPRINTF_("%s[%u]: adjusted guest_rip=%08x\n",  __func__, cpuid, guest_rip);
}



