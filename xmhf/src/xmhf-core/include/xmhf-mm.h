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

// author: Miao Yu
#ifndef __XMHF_MM_H__
#define __XMHF_MM_H__

#define XMHF_HEAP_SIZE	(64*(1<<20))
#ifndef __ASSEMBLY__

//! This struct records the information of memory allocation
//! This struct is stored in the caller provided memory allocation record list.
struct xmhf_mm_alloc_info
{
	void* 		hva;
	uint32_t 	alignment;
	size_t 		size;
};

void xmhf_mm_init(void);
void xmhf_mm_fini(void);
void* xmhf_mm_alloc_page(uint32_t num_pages);
void* xmhf_mm_malloc(size_t size);
void* xmhf_mm_malloc_align(uint32_t alignment, size_t size);
void xmhf_mm_free(void* ptr);

//! Allocate an aligned memory from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
extern void* xmhf_mm_alloc_align_with_record(XMHFList* mm_alloc_infolist, uint32_t alignment, size_t size);

//! Allocate a piece of memory from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
extern void* xmhf_mm_malloc_with_record(XMHFList* mm_alloc_infolist, size_t size);

//! Allocate a memory page from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
extern void* xmhf_mm_alloc_page_with_record(XMHFList* mm_alloc_infolist, uint32_t num_pages);

//! Free the memory allocated from the heap of XMHF. And also remove the record in the <mm_alloc_infolist>
extern void xmhf_mm_free_from_record(XMHFList* mm_alloc_infolist, void* ptr);

//! @brief Free all the recorded memory
extern void xmhf_mm_free_all_records(XMHFList* mm_alloc_infolist);


#endif // __ASSEMBLY__

#endif // __XMHF_MM_H__
