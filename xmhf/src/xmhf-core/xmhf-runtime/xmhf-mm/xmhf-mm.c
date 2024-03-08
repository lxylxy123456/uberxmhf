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

#include <xmhf.h>
#include "xmhf-tlsf.h"

static u8 g_xmhf_heap[XMHF_HEAP_SIZE] __attribute__((aligned(PAGE_SIZE_4K)));
static xmhf_tlsf_pool g_pool;

// [Ticket 156][TODO] [SecBase][SecOS] Use Slab + Page Allocator to save memory space

void xmhf_mm_init(void)
{
	g_pool = xmhf_tlsf_create(g_xmhf_heap, XMHF_HEAP_SIZE);
}

void xmhf_mm_fini(void)
{
	if(g_pool)
	{
		xmhf_tlsf_destroy(g_xmhf_heap);
		g_pool = NULL;
	}
}

void* xmhf_mm_malloc_align(uint32_t alignment, size_t size)
{
	void *p = NULL;

	p = xmhf_tlsf_memalign(g_pool, alignment, size);

	if(p)
		memset(p, 0, size);
	return p;
}

void* xmhf_mm_malloc(size_t size)
{
	return xmhf_mm_malloc_align(0, size);
}

void* xmhf_mm_alloc_page(uint32_t num_pages)
{
	return xmhf_mm_malloc_align(PAGE_SIZE_4K, num_pages * PAGE_SIZE_4K);
}


void xmhf_mm_free(void* ptr)
{
	xmhf_tlsf_free(g_pool, ptr);
}

//! Allocate an aligned memory from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
void* xmhf_mm_alloc_align_with_record(XMHFList* mm_alloc_infolist, uint32_t alignment, size_t size)
{
	void* p = NULL;
	struct xmhf_mm_alloc_info* record = NULL;

	if(!mm_alloc_infolist)
		return NULL;

	p = xmhf_mm_malloc_align(alignment, size);
	if(!p)
		return NULL;

	record = xmhf_mm_malloc(sizeof(struct xmhf_mm_alloc_info));
	if(!record)
	{
		xmhf_mm_free(p);
		return NULL;
	}

	record->hva = p;
	record->alignment = alignment;
	record->size = size;
	xmhfstl_list_enqueue(mm_alloc_infolist, record, sizeof(struct xmhf_mm_alloc_info), LIST_ELEM_PTR);

	return p;
}

//! Allocate a piece of memory from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
void* xmhf_mm_malloc_with_record(XMHFList* mm_alloc_infolist, size_t size)
{
	return xmhf_mm_alloc_align_with_record(mm_alloc_infolist, 0, size);
}

//! Allocate a memory page from the heap of XMHF. Also it records the allocation in the <mm_alloc_infolist>
void* xmhf_mm_alloc_page_with_record(XMHFList* mm_alloc_infolist, uint32_t num_pages)
{
	return xmhf_mm_alloc_align_with_record(mm_alloc_infolist, PAGE_SIZE_4K, num_pages * PAGE_SIZE_4K);
}

//! Free the memory allocated from the heap of XMHF. And also remove the record in the <mm_alloc_infolist>
void xmhf_mm_free_from_record(XMHFList* mm_alloc_infolist, void* ptr)
{
	// void* p = NULL;
	struct xmhf_mm_alloc_info* record = NULL;
	XMHFListNode* node = NULL;

	if(!mm_alloc_infolist || !ptr)
		return;

	{
		XMHFLIST_FOREACH(mm_alloc_infolist, first, next, cur)
		{
			record = (struct xmhf_mm_alloc_info*)cur->value;

			if(record->hva == ptr)
			{
				node = cur;
				break;
			}
		}
		END_XMHFLIST_FOREACH(mm_alloc_infolist);
	}

	xmhfstl_list_remove(mm_alloc_infolist, node);

	xmhf_mm_free(record->hva);
	xmhf_mm_free(record);
}

//! @brief Free all the recorded memory
void xmhf_mm_free_all_records(XMHFList* mm_alloc_infolist)
{
	struct xmhf_mm_alloc_info* record = NULL;

	if(!mm_alloc_infolist)
		return;

	{
		XMHFLIST_FOREACH(mm_alloc_infolist, first, next, cur)
		{
			record = (struct xmhf_mm_alloc_info*)cur->value;

			xmhf_mm_free(record->hva);
		}
		END_XMHFLIST_FOREACH(mm_alloc_infolist);
	}

	xmhfstl_list_clear_destroy(mm_alloc_infolist);
}
