#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "_txt_mtrrs.h"

#define PAGE_SIZE_4K 0x1000U
#define PAGE_SHIFT_4K 12
#define MSR_MTRRcap                            0x0fe
#define MSR_MTRRdefType                        0x2ff
#define MTRR_TYPE_UNCACHABLE 0
#define SINIT_MTRR_MASK         0xFFFFFF

typedef uint32_t u32;
typedef uint64_t u64;

static inline uint32_t bsrl(uint32_t mask)
{
    uint32_t   result;

    __asm__ __volatile__ ("bsrl %1,%0" : "=r" (result) : "rm" (mask) : "cc");
    return (result);
}

static inline int fls(int mask)
{
    return (mask == 0 ? mask : (int)bsrl((u32)mask) + 1);
}

uint64_t rdmsr64(uint32_t msr) {
    printf("rdmsr64(0x%x)\n", msr);
    return 0ULL;
}

void wrmsr64(uint32_t msr, uint64_t val) {
    printf("wrmsr64(0x%x, 0x%016llx)\n", msr, val);
}

/*
 * set the memory type for specified range (base to base+size)
 * to mem_type and everything else to UC
 */
bool set_mem_type(void *base, uint32_t size, uint32_t mem_type)
{
    int num_pages;
    int ndx;
    mtrr_def_type_t mtrr_def_type;
    mtrr_cap_t mtrr_cap;
    mtrr_physmask_t mtrr_physmask;
    mtrr_physbase_t mtrr_physbase;

    /*
     * disable all fixed MTRRs
     * set default type to UC
     */
    mtrr_def_type.raw = rdmsr64(MSR_MTRRdefType);
    mtrr_def_type.fe = 0;
    mtrr_def_type.type = MTRR_TYPE_UNCACHABLE;
    wrmsr64(MSR_MTRRdefType, mtrr_def_type.raw);

    /*
     * initially disable all variable MTRRs (we'll enable the ones we use)
     */
    mtrr_cap.raw = rdmsr64(MSR_MTRRcap);
    for ( ndx = 0; ndx < mtrr_cap.vcnt; ndx++ ) {
        mtrr_physmask.raw = rdmsr64(MTRR_PHYS_MASK0_MSR + ndx*2);
        mtrr_physmask.v = 0;
        wrmsr64(MTRR_PHYS_MASK0_MSR + ndx*2, mtrr_physmask.raw);
    }

    /*
     * map all AC module pages as mem_type
     */

    num_pages = (size + PAGE_SIZE_4K - 1) >> PAGE_SHIFT_4K;
    ndx = 0;

    printf("setting MTRRs for acmod: base=%p, size=%x, num_pages=%d\n",
           base, size, num_pages);

    while ( num_pages > 0 ) {
        uint32_t pages_in_range;

        /* set the base of the current MTRR */
        mtrr_physbase.raw = rdmsr64(MTRR_PHYS_BASE0_MSR + ndx*2);
        mtrr_physbase.base = ((unsigned long)base >> PAGE_SHIFT_4K) &
                             SINIT_MTRR_MASK;
        mtrr_physbase.type = mem_type;
        wrmsr64(MTRR_PHYS_BASE0_MSR + ndx*2, mtrr_physbase.raw);

        /*
         * calculate MTRR mask
         * MTRRs can map pages in power of 2
         * may need to use multiple MTRRS to map all of region
         */
        pages_in_range = 1 << (fls(num_pages) - 1);

        mtrr_physmask.raw = rdmsr64(MTRR_PHYS_MASK0_MSR + ndx*2);
        mtrr_physmask.mask = ~(pages_in_range - 1) & SINIT_MTRR_MASK;
        mtrr_physmask.v = 1;
        wrmsr64(MTRR_PHYS_MASK0_MSR + ndx*2, mtrr_physmask.raw);

        /* prepare for the next loop depending on number of pages
         * We figure out from the above how many pages could be used in this
         * mtrr. Then we decrement the count, increment the base,
         * increment the mtrr we are dealing with, and if num_pages is
         * still not zero, we do it again.
         */
        base += (pages_in_range * PAGE_SIZE_4K);
        num_pages -= pages_in_range;
        ndx++;
        if ( ndx == mtrr_cap.vcnt ) {
            printf("exceeded number of var MTRRs when mapping range\n");
            return false;
        }
    }

    printf("LXY: base=0x%08llx\n", (u64) (uintptr_t) base);
    printf("LXY: size=0x%08x\n", size);
    printf("LXY: mem_type=0x%08x\n", mem_type);

    return true;
}

int main() {
    set_mem_type((void *)0xccef0000UL, 0x00020000, 0x00000006);
}
