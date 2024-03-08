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

// author: Miao Yu [Superymk]
#ifndef XMHF_DMAP
#define XMHF_DMAP

#include <xmhf.h>

#ifndef __ASSEMBLY__

/// IOMMU PageTable information structure
typedef struct
{
	iommu_pt_t			iommu_pt_id;
	IOMMU_PT_TYPE 		type;

	/// @brief Record the allocated memory (pages)
	XMHFList* 			used_mem;

	/// @brief the root hva address of the iommu pt
	void* 				pt_root;
} IOMMU_PT_INFO;




/********* SUBARCH Specific functions *********/
/// Invalidate the IOMMU PageTable corresponding to <pt_info>
extern void iommu_vmx_invalidate_pt(IOMMU_PT_INFO* pt_info);

///
extern bool iommu_vmx_map(IOMMU_PT_INFO* pt_info, gpa_t gpa, spa_t spa, uint32_t flags);

/// Bind an IOMMU PT to a device
extern bool iommu_vmx_bind_device(IOMMU_PT_INFO* pt_info, DEVICEDESC* device);

/// Bind the untrusted OS's default IOMMU PT to a device
extern bool iommu_vmx_unbind_device(DEVICEDESC* device);

#endif // __ASSEMBLY__
#endif // XMHF_DMAP
