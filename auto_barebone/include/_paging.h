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

//paging.h - macros and definitions to assist with x86 paging schemes
// author: amit vasudevan (amitvasudevan@acm.org)
#ifndef __PAGING_H__
#define __PAGING_H__


//physical memory limit
#ifndef __ASSEMBLY__
#define ADDR_4GB 0x100000000ULL
#else
#define ADDR_4GB 0x100000000
#endif

#ifdef __i386__
    #define PAGE_MASK_4K        0xFFFFF000
    #define PAGE_MASK_1G        0xC0000000
#elif defined(__amd64__)
    #define PAGE_MASK_4K        0xFFFFFFFFFFFFF000
    #define PAGE_MASK_1G        0xFFFFFFFFC0000000
#else
    #error "Unsupported Arch"
#endif

#define ADDR64_PAGE_MASK_4K     0xFFFFFFFFFFFFF000ULL
#define ADDR64_PAGE_OFFSET_4K     0xFFFULL

// page sizes
#define PAGE_SHIFT_4K   12
#define PAGE_SHIFT_2M   21
#define PAGE_SHIFT_4M   22
#define PAGE_SHIFT_1G   30
#define PAGE_SHIFT_512G 39
#define PAGE_SHIFT_256T 42

#ifndef __ASSEMBLY__

/*
 * In x86 two architectures are supported: i386 and amd64. Physical addresses
 * are always 64-bits, so are defined with long long (1ULL). Virtual addresses
 * are 32-bits for i386 and 64-bits for amd64, so are defined with long (1UL).
 *
 * | type      | i386 | amd64 |
 * |-----------|------|-------|
 * | int       | 32   | 32    |
 * | long      | 32   | 64    |
 * | long long | 64   | 64    |
 * | hva_t     | 32   | 64    |
 * | spa_t     | 64   | 64    |
 *
 * This file defines two types of macros. PAGE_* are used for virtual addresses
 * (long, 32-bit for i386, 64-bit for amd64). PA_PAGE_* (SP stands for physical
 * address) are used for physical addresses (long long, always 64-bit). These
 * macros will check whether the operand type matches the size of virtual /
 * physical address. If the size does not match, an static assertion error will
 * fail.
 *
 * | PAGE_* macro    | PA_PAGE_* macro    | Usage                             |
 * |-----------------|--------------------|-----------------------------------|
 * | PAGE_SIZE_4K    | PA_PAGE_SIZE_4K    | Size of page (integer)            |
 * | PAGE_ALIGN_UP_4K| PA_PAGE_ALIGN_UP_4K| Align address up to page boundary |
 * | PAGE_ALIGN_4K   | PA_PAGE_ALIGN_4K   | Align address to page boundary    |
 * | PAGE_ALIGNED_4K | PA_PAGE_ALIGNED_4K | Test whether address is aligned   |
 *
 * For i386, PAGE_* macros are defined up to 1G, because larger sizes are
 * greater than 4G and are not supported by the 32-bit address space. For
 * amd64 and for PA_PAGE_* macros, all sizes are supported.
 *
 * | Subarch | PAGE_* macro               | PA_PAGE_* macro            |
 * |---------|----------------------------|----------------------------|
 * | i386    | 4K, 2M, 4M, 1G             | 4K, 2M, 4M, 1G, 512G, 256T |
 * | amd64   | 4K, 2M, 4M, 1G, 512G, 256T | 4K, 2M, 4M, 1G, 512G, 256T |
 */

/* Normal macros: u32 for i386, u64 for amd64 */
#define PAGE_SIZE_4K    (1UL << PAGE_SHIFT_4K)
#define PAGE_SIZE_2M    (1UL << PAGE_SHIFT_2M)
#define PAGE_SIZE_4M    (1UL << PAGE_SHIFT_4M)
#define PAGE_SIZE_1G    (1UL << PAGE_SHIFT_1G)

#ifdef __amd64__
#define PAGE_SIZE_512G  (1UL << PAGE_SHIFT_512G)
#define PAGE_SIZE_256T  (1UL << PAGE_SHIFT_256T)
#elif !defined(__i386__)
    #error "Unsupported Arch"
#endif /* !defined(__i386__) */

/* For physical address: definitions in u64 for both i386 and amd64 */
#define PA_PAGE_SIZE_4K     (1ULL << PAGE_SHIFT_4K)
#define PA_PAGE_SIZE_2M     (1ULL << PAGE_SHIFT_2M)
#define PA_PAGE_SIZE_4M     (1ULL << PAGE_SHIFT_4M)
#define PA_PAGE_SIZE_1G     (1ULL << PAGE_SHIFT_1G)
#define PA_PAGE_SIZE_512G   (1ULL << PAGE_SHIFT_512G)
#define PA_PAGE_SIZE_256T   (1ULL << PAGE_SHIFT_256T)

#else

#define PAGE_SIZE_4K    (1 << PAGE_SHIFT_4K)
#define PAGE_SIZE_2M    (1 << PAGE_SHIFT_2M)
#define PAGE_SIZE_4M    (1 << PAGE_SHIFT_4M)
#define PAGE_SIZE_1G    (1 << PAGE_SHIFT_1G)
#define PAGE_SIZE_512G  (1 << PAGE_SHIFT_512G)
#define PAGE_SIZE_256T  (1 << PAGE_SHIFT_256T)

#endif

/*
 * Check whether x's type satisfies prefix pf ("" or "PA_").
 *
 * The following works in most cases, but does not work in global array size.
 *  ({ _Static_assert(sizeof(x) == , "type size mismatch!"); (x); })
 *
 * So instead use sizeof(struct {_Static_assert(...)}).
 *
 * Reference: https://stackoverflow.com/a/74923016
 * Reference: https://stackoverflow.com/a/58263525
 */
#define _PAGE_TYPE_CHECK(x, pf) \
	((typeof(x)) (sizeof(struct { \
		_Static_assert(sizeof(x) == sizeof(pf##PAGE_SIZE_4K)); \
		int dummy; \
	}) * 0) + (x))

/* Align address up */

/* Align an address x up. pf is prefix ("" or "PA_"), sz is "4K", "2M", ... */
#define _PAGE_ALIGN_UP(x, pf, sz)                           \
    ((_PAGE_TYPE_CHECK((x), pf) + pf##PAGE_SIZE_##sz - 1) & \
     ~(pf##PAGE_SIZE_##sz - 1))

#define PAGE_ALIGN_UP_4K(x)         _PAGE_ALIGN_UP((x), , 4K)
#define PAGE_ALIGN_UP_2M(x)         _PAGE_ALIGN_UP((x), , 2M)
#define PAGE_ALIGN_UP_4M(x)         _PAGE_ALIGN_UP((x), , 4M)
#define PAGE_ALIGN_UP_1G(x)         _PAGE_ALIGN_UP((x), , 1G)
#ifdef __amd64__
#define PAGE_ALIGN_UP_512G(x)       _PAGE_ALIGN_UP((x), , 512G)
#define PAGE_ALIGN_UP_256T(x)       _PAGE_ALIGN_UP((x), , 256T)
#elif !defined(__i386__)
    #error "Unsupported Arch"
#endif /* !defined(__i386__) */

#define PA_PAGE_ALIGN_UP_4K(x)      _PAGE_ALIGN_UP((x), PA_, 4K)
#define PA_PAGE_ALIGN_UP_2M(x)      _PAGE_ALIGN_UP((x), PA_, 2M)
#define PA_PAGE_ALIGN_UP_4M(x)      _PAGE_ALIGN_UP((x), PA_, 4M)
#define PA_PAGE_ALIGN_UP_1G(x)      _PAGE_ALIGN_UP((x), PA_, 1G)
#define PA_PAGE_ALIGN_UP_512G(x)    _PAGE_ALIGN_UP((x), PA_, 512G)
#define PA_PAGE_ALIGN_UP_256T(x)    _PAGE_ALIGN_UP((x), PA_, 256T)

/* Align address (down) */

/* Align an address x down. pf is prefix ("" or "PA_"), sz is "4K", "2M", ... */
#define _PAGE_ALIGN(x, pf, sz)   \
    (_PAGE_TYPE_CHECK((x), pf) & ~(pf##PAGE_SIZE_##sz - 1))

#define PAGE_ALIGN_4K(x)        _PAGE_ALIGN((x), , 4K)
#define PAGE_ALIGN_2M(x)        _PAGE_ALIGN((x), , 2M)
#define PAGE_ALIGN_4M(x)        _PAGE_ALIGN((x), , 4M)
#define PAGE_ALIGN_1G(x)        _PAGE_ALIGN((x), , 1G)
#ifdef __amd64__
#define PAGE_ALIGN_512G(x)      _PAGE_ALIGN((x), , 512G)
#define PAGE_ALIGN_256T(x)      _PAGE_ALIGN((x), , 256T)
#elif !defined(__i386__)
    #error "Unsupported Arch"
#endif /* !defined(__i386__) */

#define PA_PAGE_ALIGN_4K(x)     _PAGE_ALIGN((x), PA_, 4K)
#define PA_PAGE_ALIGN_2M(x)     _PAGE_ALIGN((x), PA_, 2M)
#define PA_PAGE_ALIGN_4M(x)     _PAGE_ALIGN((x), PA_, 4M)
#define PA_PAGE_ALIGN_1G(x)     _PAGE_ALIGN((x), PA_, 1G)
#define PA_PAGE_ALIGN_512G(x)   _PAGE_ALIGN((x), PA_, 512G)
#define PA_PAGE_ALIGN_256T(x)   _PAGE_ALIGN((x), PA_, 256T)

/* Test whether address is aligned */

/* Test if x is aligned. pf is prefix ("" or "PA_"), sz is "4K", "2M", ... */
#define _PAGE_ALIGNED(x, pf, sz)   \
    (pf##PAGE_ALIGN_##sz(x) == _PAGE_TYPE_CHECK((x), pf))

#define PAGE_ALIGNED_4K(x)      _PAGE_ALIGNED((x), , 4K)
#define PAGE_ALIGNED_2M(x)      _PAGE_ALIGNED((x), , 2M)
#define PAGE_ALIGNED_4M(x)      _PAGE_ALIGNED((x), , 4M)
#define PAGE_ALIGNED_1G(x)      _PAGE_ALIGNED((x), , 1G)
#ifdef __amd64__
#define PAGE_ALIGNED_512G(x)    _PAGE_ALIGNED((x), , 512G)
#define PAGE_ALIGNED_256T(x)    _PAGE_ALIGNED((x), , 256T)
#elif !defined(__i386__)
    #error "Unsupported Arch"
#endif /* !defined(__i386__) */

#define PA_PAGE_ALIGNED_4K(x)   _PAGE_ALIGNED((x), PA_, 4K)
#define PA_PAGE_ALIGNED_2M(x)   _PAGE_ALIGNED((x), PA_, 2M)
#define PA_PAGE_ALIGNED_4M(x)   _PAGE_ALIGNED((x), PA_, 4M)
#define PA_PAGE_ALIGNED_1G(x)   _PAGE_ALIGNED((x), PA_, 1G)
#define PA_PAGE_ALIGNED_512G(x) _PAGE_ALIGNED((x), PA_, 512G)
#define PA_PAGE_ALIGNED_256T(x) _PAGE_ALIGNED((x), PA_, 256T)

#endif /* __PAGING_H__ */

