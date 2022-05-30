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

// nested-x86vmx-vmcs12.c
// Handle VMCS in the guest
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>
#include "nested-x86vmx-vmcs12.h"

int xmhf_nested_arch_x86vmx_vmcs_readable(struct nested_vmcs12 *vmcs12, void *field) {
	switch (field - (void *) vmcs12) {
#define DECLARE_FIELD_16(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_32(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_64(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_NW(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#include "nested-x86vmx-vmcs12-fields.h"
	default:
		HALT_ON_ERRORCOND(0 && "Unknown guest VMCS field");
		return -1;
	}
}

int xmhf_nested_arch_x86vmx_vmcs_writable(struct nested_vmcs12 *vmcs12, void *field) {
	switch (field - (void *) vmcs12) {
#define DECLARE_FIELD_16_RO(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 0;
#define DECLARE_FIELD_32_RO(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 0;
#define DECLARE_FIELD_64_RO(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 0;
#define DECLARE_FIELD_NW_RO(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 0;
#define DECLARE_FIELD_16_RW(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_32_RW(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_64_RW(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#define DECLARE_FIELD_NW_RW(encoding, name, extra) \
		case offsetof(struct nested_vmcs12, name): return 1;
#include "nested-x86vmx-vmcs12-fields.h"
	default:
		HALT_ON_ERRORCOND(0 && "Unknown guest VMCS field");
		return -1;
	}
}
// TODO

