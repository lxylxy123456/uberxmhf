// author: Miao Yu

#include <xmhf.h>
#include "../../../iommu-pt.h"

extern void* vtd_cet; // cet holds all its structures in the memory linearly

#define NUM_PT_ENTRIES      512 // The number of page table entries in the PML level

#define PAE_get_pdptindex(x)    ((x) >> 30) & (NUM_PT_ENTRIES - 1)
#define PAE_get_pdtindex(x)     ( (x) >> 21) & (NUM_PT_ENTRIES - 1)
#define PAE_get_ptindex(x)      ( (x) >> 12 ) & (NUM_PT_ENTRIES - 1)


//! Invalidate the IOMMU PageTable corresponding to <pt_info>
void iommu_x86vmx_invalidate_pt(IOMMU_PT_INFO* pt_info)
{
	(void)pt_info;
// [TODO] We should invalidate this page table only later
    xmhf_dmaprot_invalidate_cache();
}


/*static bool _iommu_destroy_1GB_pts(IOMMU_PT_INFO* pt_info, uint64_t pt_1GB_pte)
{

}

static void _iommu_map_1GB_page(IOMMU_PT_INFO* pt_info, void* upper_level_pts, gpa_t gpa, spa_t spa, uint32_t flags)
{
    uint32_t pt_1GB_index = 0;
    uint64_t old_1GB_pte = 0;

    pt_1GB_index = PAE_get_pdptindex(gpa);
    old_1GB_pte = ((uint64_t*)upper_level_pts)[pt_1GB_index];

    if(old_1GB_pte)
    {
        // We have set a PTE at this slot before, so we need to free the old 
        // page structures first.

        // Step 1. 
        
    }

    // Now we can safely assign the new PTE.
    ((uint64_t*)upper_level_pts)[pt_1GB_index] = (spa & PAGE_MASK_1GB) | (uint64_t)flags 
                | (uint64_t)VTD_SUPERPAGE | (uint64_t) VTD_PRESENT;

    return true;
}

bool iommu_x86vmx_map(IOMMU_PT_INFO* pt_info, gpa_t start_gpa, spa_t start_spa, uint32_t num_pages, uint32_t flags)
{
    uint32_t remained_pages = num_pages;

    // Step 1. Allocate root page table if not exist
    if(!pt_info->pt_root)
    {
        pt_info->pt_root = xmhf_mm_alloc_page_with_record(pt_info->used_mem, 1);
        if(!pt_info->pt_root)
            return false;
    }

    // Step 2. Try mapping 1GB page frame


    return true;
}

bool iommu_x86vmx_map_batch(IOMMU_PT_INFO* pt_info, gpa_t start_gpa, spa_t* spas, uint32_t num_pages, uint32_t flags)
{

    // Step 1. Allocate root page table if not exist
    if(!pt_info->pt_root)
    {
        pt_info->pt_root = xmhf_mm_alloc_page_with_record(pt_info->used_mem, 1);
        if(!pt_info->pt_root)
            return false;
    }

    return true;
}*/

static void* __vtd_get_l1pt(IOMMU_PT_INFO* pt_info)
{
	// Step 1. Allocate root page table if not exist
	if(!pt_info->pt_root)
	{
        pt_info->pt_root = xmhf_mm_alloc_page_with_record(pt_info->used_mem, 1);
		if(!pt_info->pt_root)
			printf("[VTD-ERROR] No memory to allocate IOMMU top-level page struct!\n");
	}
	return pt_info->pt_root;
}

static void* __vtd_get_nextlvl_pt(IOMMU_PT_INFO* pt_info, void* pt_base, uint32_t pt_idx)
{
	uint64_t* p_pte = &((uint64_t*)pt_base)[pt_idx];
	void* nextlvl_pt = NULL;

	if(!*p_pte)
	{
		nextlvl_pt = xmhf_mm_alloc_page_with_record(pt_info->used_mem, 1);
		if(!nextlvl_pt)
		{
			printf("[VTD-ERROR] No memory to allocate next level IOMMU page struct!\n");
			return NULL;
		}

		*p_pte = (hva2spa(nextlvl_pt) & PAGE_MASK_4K) | ((uint64_t)VTD_READ | (uint64_t)VTD_WRITE);
	}

	return spa2hva(*p_pte & PAGE_MASK_4K);
	
}

bool iommu_x86vmx_map(IOMMU_PT_INFO* pt_info, gpa_t gpa, spa_t spa, uint32_t flags)
{
	void *pdp = NULL, *pd = NULL, *pt = NULL;
	uint32_t pdp_idx, pd_idx, pt_idx = 0;

	pdp_idx = PAE_get_pdptindex(gpa);
	pd_idx = PAE_get_pdtindex(gpa);
	pt_idx = PAE_get_ptindex(gpa);
	
	// Step 1. Get PDP
	pdp = __vtd_get_l1pt(pt_info);
	if(!pdp)
		return false;

	// Step 2. Get PD
	pd = __vtd_get_nextlvl_pt(pt_info, pdp, pdp_idx);
	if(!pd)
		return false;

	// Step 3. Get PT
	pt = __vtd_get_nextlvl_pt(pt_info, pd, pd_idx);
	if(!pt)
		return false;

	// Step 4. Map spa
	if(flags == DMA_ALLOW_ACCESS)
        ((uint64_t*)pt)[pt_idx] = (spa & PAGE_MASK_4K) | ((uint64_t)VTD_READ | (uint64_t)VTD_WRITE);
    else if (flags == DMA_DENY_ACCESS)
        ((uint64_t*)pt)[pt_idx] = (spa & PAGE_MASK_4K) & (uint64_t)0xfffffffe; // remove the present bit


	return true;
}

static void __x86vmx_bind_cet(DEVICEDESC* device, iommu_pt_t pt_id, spa_t iommu_pt_root)
{
	uint64_t *value;

	// Update the CET
	value = (uint64_t *)((hva_t)vtd_cet + (device->bus * PAGE_SIZE_4K) + 
		(device->dev * PCI_FUNCTION_MAX + device->func) * 16);
	*(value + 1) = (uint64_t)0x0000000000000001ULL | (((uint64_t)pt_id & 0xFFFF) << 8); //domain:<pt_id>, aw=39 bits, 3 level pt
	*value = iommu_pt_root;
	*value |= 0x1ULL; //present, enable fault recording/processing, multilevel pt translation
}

bool iommu_x86vmx_bind_device(IOMMU_PT_INFO* pt_info, DEVICEDESC* device)
{
	__x86vmx_bind_cet(device, pt_info->iommu_pt_id, hva2spa(pt_info->pt_root));
	
	return true;
}

//! Bind the untrusted OS's default IOMMU PT to a device
bool iommu_x86vmx_unbind_device(DEVICEDESC* device)
{
	__x86vmx_bind_cet(device, UNTRUSTED_OS_IOMMU_PT_ID, hva2spa(g_rntm_dmaprot_buffer));

	return true;
}