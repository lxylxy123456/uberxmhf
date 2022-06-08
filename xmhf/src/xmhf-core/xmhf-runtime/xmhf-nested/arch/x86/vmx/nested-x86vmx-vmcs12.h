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

// nested-x86vmx-vmcs12.h
// Handle VMCS in the guest
// author: Eric Li (xiaoyili@andrew.cmu.edu)

#ifndef _NESTED_X86VMX_VMCS12_H_
#define _NESTED_X86VMX_VMCS12_H_

struct nested_vmcs12 {
#define DECLARE_FIELD_16(encoding, name, ...) \
	u16 name;
#define DECLARE_FIELD_64(encoding, name, ...) \
	u64 name;
#define DECLARE_FIELD_32(encoding, name, ...) \
	u32 name;
#define DECLARE_FIELD_NW(encoding, name, ...) \
	ulong_t name;
#include "nested-x86vmx-vmcs12-fields.h"
};

size_t xmhf_nested_arch_x86vmx_vmcs_field_find(ulong_t encoding);
int xmhf_nested_arch_x86vmx_vmcs_writable(size_t offset);
ulong_t xmhf_nested_arch_x86vmx_vmcs_read(struct nested_vmcs12 *vmcs12,
											size_t offset, size_t size);
void xmhf_nested_arch_x86vmx_vmcs_write(struct nested_vmcs12 *vmcs12,
										size_t offset, ulong_t value,
										size_t size);
void xmhf_nested_arch_x86vmx_vmcs_dump(VCPU *vcpu, struct nested_vmcs12 *vmcs12,
										char *prefix);
void xmhf_nested_arch_x86vmx_vmread_all(VCPU *vcpu, char *prefix);
u32 xmhf_nested_arch_x86vmx_vmcs12_to_vmcs02(VCPU *vcpu,
											struct nested_vmcs12 *vmcs12);
void xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12(VCPU *vcpu,
												struct nested_vmcs12 *vmcs12);

#endif /* _NESTED_X86VMX_VMCS12_H_ */
