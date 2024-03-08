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

#ifndef INCLUDED_xmhf_tlsf
#define INCLUDED_xmhf_tlsf

/*
** Two Level Segregated Fit memory allocator, version 1.9.
** Written by Matthew Conte, and placed in the Public Domain.
**	http://xmhf_tlsf.baisoku.org
**
** Based on the original documentation by Miguel Masmano:
**	http://rtportal.upv.es/rtmalloc/allocators/xmhf_tlsf/index.shtml
**
** Please see the accompanying Readme.txt for implementation
** notes and caveats.
**
** This implementation was written to the specification
** of the document, therefore no GPL restrictions apply.
*/

#include <stdio.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Create/destroy a memory pool. */
typedef void* xmhf_tlsf_pool;
xmhf_tlsf_pool xmhf_tlsf_create(void* mem, size_t bytes);
void xmhf_tlsf_destroy(xmhf_tlsf_pool pool);

/* malloc/memalign/realloc/free replacements. */
void* xmhf_tlsf_malloc(xmhf_tlsf_pool pool, size_t bytes);
void* xmhf_tlsf_memalign(xmhf_tlsf_pool pool, size_t align, size_t bytes);
void* xmhf_tlsf_realloc(xmhf_tlsf_pool pool, void* ptr, size_t size);
void xmhf_tlsf_free(xmhf_tlsf_pool pool, void* ptr);

/* Debugging. */
typedef void (*xmhf_tlsf_walker)(void* ptr, size_t size, int used, void* user);
void xmhf_tlsf_walk_heap(xmhf_tlsf_pool pool, xmhf_tlsf_walker walker, void* user);
/* Returns nonzero if heap check fails. */
int xmhf_tlsf_check_heap(xmhf_tlsf_pool pool);

/* Returns internal block size, not original request size */
size_t xmhf_tlsf_block_size(void* ptr);

/* Overhead of per-pool internal structures. */
size_t xmhf_tlsf_overhead(void);

#if defined(__cplusplus)
};
#endif

#endif
