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


#define PAGE_MASK_4K				0xfffff000
#define PAGE_MASK_1G       0xC0000000

// page sizes
#define PAGE_SHIFT_4K   12
#define PAGE_SHIFT_2M   21
#define PAGE_SHIFT_4M   22
#define PAGE_SHIFT_1G   30
#define PAGE_SHIFT_512G 39
#define PAGE_SHIFT_256T 42

#ifndef __ASSEMBLY__
#define PAGE_SIZE_4K    (1ULL << PAGE_SHIFT_4K)
#define PAGE_SIZE_2M    (1ULL << PAGE_SHIFT_2M)
#define PAGE_SIZE_4M    (1ULL << PAGE_SHIFT_4M)
#define PAGE_SIZE_1G    (1ULL << PAGE_SHIFT_1G)
#define PAGE_SIZE_512G  (1ULL << PAGE_SHIFT_512G)
#define PAGE_SIZE_256T  (1ULL << PAGE_SHIFT_256T)
#else   
#define PAGE_SIZE_4K    (1 << PAGE_SHIFT_4K)
#define PAGE_SIZE_2M    (1 << PAGE_SHIFT_2M)
#define PAGE_SIZE_4M    (1 << PAGE_SHIFT_4M)
#define PAGE_SIZE_1G    (1 << PAGE_SHIFT_1G)
#define PAGE_SIZE_512G  (1 << PAGE_SHIFT_512G)
#define PAGE_SIZE_256T  (1 << PAGE_SHIFT_256T)
#endif

#define PAGE_ALIGN_UP4K(size)   (((size) + PAGE_SIZE_4K - 1) & ~(PAGE_SIZE_4K - 1))
#define PAGE_ALIGN_UP2M(size)   (((size) + PAGE_SIZE_2M - 1) & ~(PAGE_SIZE_2M - 1))
#define PAGE_ALIGN_UP4M(size)   (((size) + PAGE_SIZE_4M - 1) & ~(PAGE_SIZE_4M - 1))
#define PAGE_ALIGN_UP1G(size)   (((size) + PAGE_SIZE_1G - 1) & ~(PAGE_SIZE_1G - 1))
#define PAGE_ALIGN_UP512G(size) (((size) + PAGE_SIZE_512G - 1) & ~(PAGE_SIZE_512G - 1))
#define PAGE_ALIGN_UP256T(size) (((size) + PAGE_SIZE_256T - 1) & ~(PAGE_SIZE_256T - 1))

#define PAGE_ALIGN_4K(size)     ((size) & ~(PAGE_SIZE_4K - 1))
#define PAGE_ALIGN_2M(size)     ((size) & ~(PAGE_SIZE_2M - 1))
#define PAGE_ALIGN_4M(size)     ((size) & ~(PAGE_SIZE_4M - 1))
#define PAGE_ALIGN_1G(size)     ((size) & ~(PAGE_SIZE_1G - 1))
#define PAGE_ALIGN_512G(size)   ((size) & ~(PAGE_SIZE_512G - 1))
#define PAGE_ALIGN_256T(size)   ((size) & ~(PAGE_SIZE_256T - 1))

#define PAGE_ALIGNED_4K(size)   (PAGE_ALIGN_4K(size) == size)
#define PAGE_ALIGNED_2M(size)   (PAGE_ALIGN_2M(size) == size)
#define PAGE_ALIGNED_4M(size)   (PAGE_ALIGN_4M(size) == size)
#define PAGE_ALIGNED_1G(size)   (PAGE_ALIGN_1G(size) == size)
#define PAGE_ALIGNED_512G(size) (PAGE_ALIGN_512G(size) == size)
#define PAGE_ALIGNED_256T(size) (PAGE_ALIGN_256T(size) == size)

#define BYTES_TO_PAGE4K(size)	(PAGE_ALIGN_UP4K(size) >> PAGE_SHIFT_4K)

// non-PAE mode specific definitions 
#define NPAE_PTRS_PER_PDT       1024
#define NPAE_PTRS_PER_PT        1024
#define NPAE_PAGETABLE_SHIFT    12
#define NPAE_PAGEDIR_SHIFT      22
#define NPAE_PAGE_DIR_MASK      0xffc00000
#define NPAE_PAGE_TABLE_MASK    0x003ff000

// PAE mode specific definitions 
#define PAE_PTRS_PER_PDPT  4
#define PAE_PTRS_PER_PDT   512
#define PAE_PTRS_PER_PT    512
#define PAE_PT_SHIFT       12
#define PAE_PDT_SHIFT      21
#define PAE_PDPT_SHIFT     30
#define PAE_PT_MASK        0x001ff000
#define PAE_PDT_MASK       0x3fe00000
#define PAE_PDPT_MASK      0xc0000000
#define PAE_ENTRY_SIZE     8

// 4-level paging specific definitions
#define P4L_NPLM4T  (PAGE_ALIGN_UP256T(MAX_PHYS_ADDR) >> PAGE_SHIFT_256T)
#define P4L_NPDPT   (PAGE_ALIGN_UP512G(MAX_PHYS_ADDR) >> PAGE_SHIFT_512G)
#define P4L_NPDT    (PAGE_ALIGN_UP1G(MAX_PHYS_ADDR) >> PAGE_SHIFT_1G)
#define P4L_NPT     (PAGE_ALIGN_UP2M(MAX_PHYS_ADDR) >> PAGE_SHIFT_2M)

// various paging flags 
#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_PWT           3
#define _PAGE_BIT_PCD           4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6
#define _PAGE_BIT_PSE           7
#define _PAGE_BIT_GLOBAL        8
#define _PAGE_BIT_UNUSED1       9
#define _PAGE_BIT_UNUSED2       10
#define _PAGE_BIT_UNUSED3       11
#define _PAGE_BIT_NX            63

#define _PAGE_PRESENT   0x001
#define _PAGE_RW        0x002   
#define _PAGE_USER      0x004
#define _PAGE_PWT       0x008
#define _PAGE_PCD       0x010
#define _PAGE_ACCESSED  0x020
#define _PAGE_DIRTY     0x040
#define _PAGE_PSE       0x080
#define _PAGE_GLOBAL    0x100
#define _PAGE_UNUSED1   0x200
#define _PAGE_UNUSED2   0x400
#define _PAGE_UNUSED3   0x800

#ifndef __ASSEMBLY__
#define _PAGE_NX        (1ULL<<_PAGE_BIT_NX)
#else
#define _PAGE_NX  (1 << _PAGE_BIT_NX)
#endif 

#define PAGE_FAULT_BITS (_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_NX)

#ifndef __ASSEMBLY__

typedef u64 pml4te_t;
typedef u64 pdpte_t;
typedef u64 pdte_t;
typedef u64 pte_t;

typedef pml4te_t *pml4t_t;
typedef pdpte_t *pdpt_t;
typedef pdte_t *pdt_t;
typedef pte_t *pt_t;

typedef u32 *npdt_t;
typedef u32 *npt_t;

/* 4-level paging macros */

/* make a PML4 entry from individual fields */
#define p4l_make_plm4e(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page directory pointer entry from individual fields */
#define p4l_make_pdpe(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page directory entry for a 2MB page from individual fields */
#define p4l_make_pde_big(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_2M - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page directory entry for a 4KB page from individual fields */
#define p4l_make_pde(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page table entry from individual fields */
#define p4l_make_pte(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* get address field from 64-bit cr3 (page directory pointer) */
#define p4l_get_addr_from_32bit_cr3(entry) \
  ((u64)(entry) & (~((1UL << 12) - 1)))

/* get flags field from 64-bit cr3 (page directory pointer) */
#define p4l_get_flags_from_32bit_cr3(entry) \
  ((u64)(entry) & ((1UL << 12) - 1))

/* get address field of a PML4E (page directory pointer entry) */
#define p4l_get_addr_from_pml4e(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a PML4E (page directory pointer entry) */
#define p4l_get_flags_from_pml4e(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* get address field of a pdpe (page directory pointer entry) */
#define p4l_get_addr_from_pdpe(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a pdpe (page directory pointer entry) */
#define p4l_get_flags_from_pdpe(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* get address field of a 2MB pdte (page directory entry) */
#define p4l_get_addr_from_pde_big(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_2M - 1) | _PAGE_NX)))

/* get flags field of a 2MB pdte (page directory entry) */
#define p4l_get_flags_from_pde_big(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_2M - 1) | _PAGE_NX))

/* get address field of a 4K pdte (page directory entry) */
#define p4l_get_addr_from_pde(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a 4K pdte (page directory entry) */
#define p4l_get_flags_from_pde(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* get address field of a pte (page table entry) */
#define p4l_get_addr_from_pte(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a pte (page table entry) */
#define p4l_get_flags_from_pte(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* 32-bit macros */

/* make a page directory pointer entry from individual fields */
#define pae_make_pdpe(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page directory entry for a 2MB page from individual fields */
#define pae_make_pde_big(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_2M - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page directory entry for a 4KB page from individual fields */
#define pae_make_pde(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* make a page table entry from individual fields */
#define pae_make_pte(paddr, flags) \
  ((u64)(paddr) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) | (u64)(flags)

/* get address field from 32-bit cr3 (page directory pointer) in PAE mode */
#define pae_get_addr_from_32bit_cr3(entry) \
  ((u32)(entry) & (~((1UL << 5) - 1))) 

/* get flags field from 32-bit cr3 (page directory pointer) in PAE mode */
#define pae_get_flags_from_32bit_cr3(entry) \
  ((u32)(entry) & ((1UL << 5) - 1)) 

/* get address field of a pdpe (page directory pointer entry) */
#define pae_get_addr_from_pdpe(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))) 

/* get flags field of a pdpe (page directory pointer entry) */
#define pae_get_flags_from_pdpe(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)) 

/* get address field of a 2MB pdte (page directory entry) */
#define pae_get_addr_from_pde_big(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_2M - 1) | _PAGE_NX)))

/* get flags field of a 2MB pdte (page directory entry) */
#define pae_get_flags_from_pde_big(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_2M - 1) | _PAGE_NX))

/* get address field of a 4K pdte (page directory entry) */
#define pae_get_addr_from_pde(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a 4K pdte (page directory entry) */
#define pae_get_flags_from_pde(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* get address field of a pte (page table entry) */
#define pae_get_addr_from_pte(entry) \
  ((u64)(entry) & (~(((u64)PAGE_SIZE_4K - 1) | _PAGE_NX)))

/* get flags field of a pte (page table entry) */
#define pae_get_flags_from_pte(entry) \
  ((u64)(entry) & (((u64)PAGE_SIZE_4K - 1) | _PAGE_NX))

/* make a page directory entry for a 4MB page from individual fields */
#define npae_make_pde_big(paddr, flags) \
  ((u32)(paddr) & (~(((u32)PAGE_SIZE_4M - 1)))) | (u32)(flags)

/* make a page directory entry for a 4KB page from individual fields */
#define npae_make_pde(paddr, flags) \
  ((u32)(paddr) & (~(((u32)PAGE_SIZE_4K - 1)))) | (u32)(flags)

/* get address field from NON-PAE cr3 (page directory pointer) */
#define npae_get_addr_from_32bit_cr3(entry) \
  ((u32)(entry) & (~((u32)PAGE_SIZE_4K - 1))) 

/* get address field of a 4K non-PAE pdte (page directory entry) */
#define npae_get_addr_from_pde(entry) \
  ((u32)(entry) & (~((u32)PAGE_SIZE_4K - 1)))

/* get flags field of a 4K non-PAE pdte (page directory entry) */
#define npae_get_flags_from_pde(entry) \
  ((u32)(entry) & ((u32)PAGE_SIZE_4K - 1))

/* get address field of a 4M (non-PAE) pdte (page directory entry) */
#define npae_get_addr_from_pde_big(entry) \
  ((u32)(entry) & (~((u32)PAGE_SIZE_4M - 1)))

/* get flags field of a 4M (non-PAE) pdte (page directory entry) */
#define npae_get_flags_from_pde_big(entry) \
  ((u32)(entry) & ((u32)PAGE_SIZE_4M - 1))

/* get flags field of a pte (page table entry) */
#define npae_get_flags_from_pte(entry) \
  ((u32)(entry) & (((u32)PAGE_SIZE_4K - 1)))

/* get address field of a 4K (non-PAE) pte (page table entry) */
#define npae_get_addr_from_pte(entry) \
  ((u32)(entry) & (~((u32)PAGE_SIZE_4K - 1)))

/* get page offset field of a vaddr in a 4K page (PAE mode) */
#define pae_get_offset_4K_page(entry) \
  ((u32)(entry) & ((u32)PAGE_SIZE_4K - 1))

/* get page offset field of a vaddr in a 2M page */
#define pae_get_offset_big(entry) \
  ((u32)(entry) & ((u32)PAGE_SIZE_2M - 1))

/* get offset field of a vaddr in a 4K page (non-PAE mode) */
#define npae_get_offset_4K_page(vaddr) \
  ((u32)(vaddr) & ((u32)PAGE_SIZE_4K - 1))

/* get offset field of a vaddr in a 4M page */
#define npae_get_offset_big(vaddr) \
  ((u32)(vaddr) & ((u32)PAGE_SIZE_4M - 1))

/* get index field of a paddr in a pdp level */
#define pae_get_pdpt_index(paddr)\
    (((paddr) & PAE_PDPT_MASK) >> PAE_PDPT_SHIFT)

/* get index field of a paddr in a pd level */
#define pae_get_pdt_index(paddr)\
    (((paddr) & PAE_PDT_MASK) >> PAE_PDT_SHIFT)

/* get index field of a paddr in a pt level */
#define pae_get_pt_index(paddr)\
    (((paddr) & PAE_PT_MASK) >> PAE_PT_SHIFT)

/* get index field of a paddr in a pd level */
#define npae_get_pdt_index(paddr)\
    (((paddr) & NPAE_PAGE_DIR_MASK) >> NPAE_PAGEDIR_SHIFT)

/* get index field of a paddr in a pt level */
#define npae_get_pt_index(paddr)\
    (((paddr) & NPAE_PAGE_TABLE_MASK) >> NPAE_PAGETABLE_SHIFT)

#define set_exec(entry) (((u64)entry) &= (~((u64)_PAGE_NX)))
#define set_nonexec(entry) (((u64)entry) |= ((u64)_PAGE_NX))
#define set_readonly(entry) (((u64)entry) &= (~((u64)_PAGE_RW)))
#define set_readwrite(entry) (((u64)entry) |= ((u64)_PAGE_RW))
#define set_present(entry) (((u64)entry) |= ((u64)_PAGE_PRESENT))
#define set_not_present(entry) (((u64)entry) &= (~((u64)_PAGE_PRESENT)))

#define SAME_PAGE_PAE(addr1, addr2)		\
    (((addr1) >> PAE_PT_SHIFT) == ((addr2) >> PAE_PT_SHIFT))

#define SAME_PAGE_NPAE(addr1, addr2)		\
    (((addr1) >> NPAE_PAGETABLE_SHIFT) == ((addr2) >> NPAE_PAGETABLE_SHIFT))


#define CACHE_WBINV()  __asm__ __volatile__("wbinvd\n" :::"memory")
#define TLB_INVLPG(x) __asm__ __volatile__("invlpg (%0)\n": /* no output */ : "r" (x): "memory")


#endif /* __ASSEMBLY__ */

#endif /* __PAGING_H__ */
