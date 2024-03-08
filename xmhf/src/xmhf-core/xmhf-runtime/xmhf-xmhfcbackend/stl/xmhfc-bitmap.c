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

#include <xmhf.h>

XMHF_STL_BITMAP* xmhfstl_bitmap_create(uint32_t num_bits)
{
	XMHF_STL_BITMAP* result = NULL;
	ulong_t num_pages = 0;

	result = (XMHF_STL_BITMAP*)xmhf_mm_malloc(sizeof(XMHF_STL_BITMAP));
	if(!result)
		return NULL;

	// TODO: change num_bits's type to ulong_t, then remove type cast
	num_pages = BYTES_TO_PAGE4K((ulong_t)BITS_TO_BYTES(num_bits));
	result->max_bits = num_bits;
	result->mem_table = (char**)xmhf_mm_malloc(num_pages * sizeof(char*));
	if(!result->mem_table)
		return NULL;

	result->bits_stat = (uint16_t*)xmhf_mm_malloc(num_pages * sizeof(uint16_t));
	if(!result->bits_stat)
		return NULL;
	return result;
}

void xmhfstl_bitmap_destroy(XMHF_STL_BITMAP* bitmap)
{
	ulong_t num_pages;
	ulong_t i = 0;

	if(!bitmap)
		return;

	// TODO: change bitmap->max_bits's type to ulong_t, then remove type cast
	num_pages = BYTES_TO_PAGE4K((ulong_t)BITS_TO_BYTES(bitmap->max_bits));
	for(i = 0; i < num_pages; i++)
	{
		char* mem = bitmap->mem_table[i];

		if(mem)
		{
			xmhf_mm_free(mem);
			mem = NULL;
		}
	}

	if(bitmap->mem_table)
	{
		xmhf_mm_free(bitmap->mem_table);
		bitmap->mem_table = NULL;
	}

	if(bitmap->bits_stat)
	{
		xmhf_mm_free(bitmap->bits_stat);
		bitmap->mem_table = NULL;
	}

	xmhf_mm_free(bitmap);
}

bool xmhfstl_bitmap_set_bit(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx)
{
	uint32_t bit_offset;
	uint32_t byte_offset;
	uint32_t pg_offset;
	uint32_t bits_stat;
	char test_bit;

	// Sanity check
	if(!bitmap)
		return false;

	if(bit_idx >= bitmap->max_bits)
		return false;

	bit_offset = bit_idx % BITS_PER_BYTE;
	byte_offset = BITS_TO_BYTES(bit_idx) % PAGE_SIZE_4K;
	pg_offset = BITS_TO_BYTES(bit_idx) >> PAGE_SHIFT_4K;

	bits_stat = bitmap->bits_stat[pg_offset];

	if(!bits_stat)
	{
		// There is no page to hold the bitmap content
		bitmap->mem_table[pg_offset] = (char*) xmhf_mm_malloc_align(PAGE_SIZE_4K, PAGE_SIZE_4K);
		if(!bitmap->mem_table[pg_offset])
			return false;
	}

	test_bit = bitmap->mem_table[pg_offset][byte_offset];
	if(test_bit & (1 << bit_offset))
		return true;  // already set
	else
		bitmap->mem_table[pg_offset][byte_offset] = (char)(test_bit | (1 << bit_offset));
	bitmap->bits_stat[pg_offset] = bits_stat + 1;

	return true;
}


bool xmhfstl_bitmap_clear_bit(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx)
{
	uint32_t bit_offset;
	uint32_t byte_offset;
	uint32_t pg_offset;
	uint32_t bits_stat;
	char test_bit;

	// Sanity check
	if(!bitmap)
		return false;

	if(bit_idx >= bitmap->max_bits)
		return false;

	bit_offset = bit_idx % BITS_PER_BYTE;
	byte_offset = BITS_TO_BYTES(bit_idx) % PAGE_SIZE_4K;
	pg_offset = BITS_TO_BYTES(bit_idx) >> PAGE_SHIFT_4K;
	bits_stat = bitmap->bits_stat[pg_offset];

	if(!bits_stat)
		return true;

	test_bit = bitmap->mem_table[pg_offset][byte_offset];
	if(test_bit & (1 << bit_offset))
	{
		// The bit is set, so we need to clear it in the next
		bitmap->mem_table[pg_offset][byte_offset] = (char)(test_bit & ~(1 << bit_offset));
		bitmap->bits_stat[pg_offset] = bits_stat - 1;
	}

	if(!bits_stat && bitmap->mem_table[pg_offset])
	{
		// There is no page to hold the xmhfstl_bitmap content
		xmhf_mm_free(bitmap->mem_table[pg_offset]);
		bitmap->mem_table[pg_offset] = NULL;
	}

	return true;
}

bool xmhfstl_bitmap_is_bit_set(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx)
{
	uint32_t bit_offset;
	uint32_t byte_offset;
	uint32_t pg_offset;

	bit_offset = bit_idx % BITS_PER_BYTE;
	byte_offset = BITS_TO_BYTES(bit_idx) % PAGE_SIZE_4K;
	pg_offset = BITS_TO_BYTES(bit_idx) >> PAGE_SHIFT_4K;

	if( !bitmap->mem_table[pg_offset])
		return false;

	if(bitmap->mem_table[pg_offset][byte_offset] & (1 << bit_offset))
		return true;
	else
		return false;
}

bool xmhfstl_bitmap_is_bit_clear(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx)
{
	uint32_t bit_offset;
	uint32_t byte_offset;
	uint32_t pg_offset;

	bit_offset = bit_idx % BITS_PER_BYTE;
	byte_offset = BITS_TO_BYTES(bit_idx) % PAGE_SIZE_4K;
	pg_offset = BITS_TO_BYTES(bit_idx) >> PAGE_SHIFT_4K;

	if(!bitmap->mem_table[pg_offset])
		return true;

	if(bitmap->mem_table[pg_offset][byte_offset] & (1 << bit_offset))
		return false;
	else
		return true;
}
