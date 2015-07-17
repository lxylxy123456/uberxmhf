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
 * vft slab stubs verification driver
 * author: amit vasudevan (amitvasudevan@acm.org)
*/


#include <xmhf.h>
#include <xmhf-hwm.h>
#include <xmhfgeec.h>
#include <xmhf-debug.h>

u32 cpuid = 0;	//BSP cpu
slab_params_t sp;

void main(void){
	u32 *stackelem;

	// upon control transfer to a slab entry stub the stack is
	// setup by the sentinel as follows:
	// TOS: <return eip>, slab_params_t *

	//@assert _slab_tos[cpuid] == (u32)&_slab_stack[1];
	sp.slab_ctype = 0;
	sp.src_slabid = 0;
	sp.dst_slabid = 0;
	sp.dst_uapifn = 0;
	sp.cpuid = cpuid;

	_slab_tos[cpuid] -= sizeof(u32);

	stackelem = (u32 *)_slab_tos[cpuid];
	//@assert (u32)(stackelem) == (u32)_slab_tos[cpuid];

        *stackelem = (u32)&sp;

	//@assert *((u32 *)(_slab_tos[cpuid])) == (u32)&sp;

	xmhfhwm_cpu_gprs_esp = _slab_tos[cpuid];

	CASM_FUNCCALL(_slab_entrystub, CASM_NOPARAM);

	//@assert xmhfhwm_cpu_gprs_esp == (u32)&_slab_stack[1];

}


