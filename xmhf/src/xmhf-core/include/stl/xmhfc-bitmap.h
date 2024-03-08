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

// Bitmap with optimized space and time complexity.  The xmhfstl_bitmap allocates/revoke
// space on demand (PAGE granularity), but adds a little performance overhead
// in the critical path

#ifndef XMHFSTL_BITMAP
#define XMHFSTL_BITMAP

#define BITS_PER_BYTE_SHIFT		3
#define BITS_PER_BYTE			(1 << BITS_PER_BYTE_SHIFT)
#define BITS_TO_BYTES(x)		((x) >> BITS_PER_BYTE_SHIFT)
#define BYTES_TO_BITS(x)		((x) << BITS_PER_BYTE_SHIFT)

#ifndef __ASSEMBLY__

// A xmhfstl_bitmap is allocated in separate memory pages. So we track the use of each
// xmhfstl_bitmap page to allocate/revoke memory on demand
typedef struct{
	uint32_t max_bits;
	char** mem_table; // Contains page aligned addresses for holding xmhfstl_bitmap content
	uint16_t* bits_stat; // num of set bit in the corresponding xmhfstl_bitmap page
}XMHF_STL_BITMAP;

extern XMHF_STL_BITMAP* xmhfstl_bitmap_create(uint32_t num_bits);
extern void xmhfstl_bitmap_destroy(XMHF_STL_BITMAP* bitmap);

extern bool xmhfstl_bitmap_set_bit(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx);
extern bool xmhfstl_bitmap_clear_bit(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx);

extern bool xmhfstl_bitmap_is_bit_set(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx);
extern bool xmhfstl_bitmap_is_bit_clear(XMHF_STL_BITMAP* bitmap, const uint32_t bit_idx);



#endif // __ASSEMBLY__

#endif // XMHFSTL_BITMAP
