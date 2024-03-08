/*
 * The following license and copyright notice applies to all content in
 * this repository where some other license does not take precedence. In
 * particular, notices in sub-project directories and individual source
 * files take precedence over this file.
 *
 * See COPYING for more information.
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2023 Eric Li
 * All Rights Reserved.
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
 * Neither the name of the copyright holder nor the names of
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
 */

#include <stdint.h>

__attribute__((always_inline))
inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	uint32_t a = 0x7a567254U, c = 0U;
#ifdef VMCALL_OFFSET
	c += VMCALL_OFFSET;
#endif
	asm volatile("cpuid" : "=a"(*(eax)), "=b"(*(ebx)), "=c"(*(ecx)),
					"=d"(*(edx)) : "0"(a), "2"(c));
}

static inline uintptr_t vmcall(uintptr_t eax, uintptr_t ecx, uintptr_t edx,
								uintptr_t esi, uintptr_t edi) {
#ifdef VMCALL_OFFSET
	eax += VMCALL_OFFSET;
#endif
	asm volatile ("vmcall\n\t" : "+a"(eax) : "c"(ecx), "d"(edx), "S"(esi),
					"D"(edi));
	return eax;
}

/* Return whether TrustVisor is present */
static inline int check_cpuid() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(&eax, &ebx, &ecx, &edx);
	if (eax == 0x7a767274U) {
		return 1;
	}
	return 0;
}
