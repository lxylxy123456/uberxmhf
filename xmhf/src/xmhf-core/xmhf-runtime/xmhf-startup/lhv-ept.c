#include <xmhf.h>
#include <lhv.h>

#define EPT_POOL_SIZE 128

/* Whether the EPT is already built. If so, store EPTP */
static u8 ept_valid[MAX_VCPU_ENTRIES][LHV_EPT_COUNT];

/* Root EPT pages */
static u8 ept_root[MAX_VCPU_ENTRIES][LHV_EPT_COUNT][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

/* Physical memory for storing more  */
static u8 ept_pool[MAX_VCPU_ENTRIES][LHV_EPT_COUNT][EPT_POOL_SIZE][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

/* Indicate whether the page in ept_pool is free */
static u8 ept_alloc[MAX_VCPU_ENTRIES][LHV_EPT_COUNT][EPT_POOL_SIZE];

/* Memory to be mapped */
static u8 ept_target[256][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

/* Large pages to be swapped */
u8 large_pages[2][512 * 4096] __attribute__((aligned(512 * 4096)));

typedef struct {
	hptw_ctx_t ctx;
	u8 (*page_pool)[PAGE_SIZE_4K];
	u8 *page_alloc;
} lhv_ept_ctx_t;

// Structure that captures fixed MTRR properties
struct _fixed_mtrr_prop_t {
	u32 msr;	/* MSR register address (ECX in RDMSR / WRMSR) */
	u32 start;	/* Start address */
	u32 step;	/* Size of each range */
	u32 end;	/* End address (start + 8 * step == end) */
} fixed_mtrr_prop[NUM_FIXED_MTRRS] = {
	{IA32_MTRR_FIX64K_00000, 0x00000000, 0x00010000, 0x00080000},
	{IA32_MTRR_FIX16K_80000, 0x00080000, 0x00004000, 0x000A0000},
	{IA32_MTRR_FIX16K_A0000, 0x000A0000, 0x00004000, 0x000C0000},
	{IA32_MTRR_FIX4K_C0000, 0x000C0000, 0x00001000, 0x000C8000},
	{IA32_MTRR_FIX4K_C8000, 0x000C8000, 0x00001000, 0x000D0000},
	{IA32_MTRR_FIX4K_D0000, 0x000D0000, 0x00001000, 0x000D8000},
	{IA32_MTRR_FIX4K_D8000, 0x000D8000, 0x00001000, 0x000E0000},
	{IA32_MTRR_FIX4K_E0000, 0x000E0000, 0x00001000, 0x000E8000},
	{IA32_MTRR_FIX4K_E8000, 0x000E8000, 0x00001000, 0x000F0000},
	{IA32_MTRR_FIX4K_F0000, 0x000F0000, 0x00001000, 0x000F8000},
	{IA32_MTRR_FIX4K_F8000, 0x000F8000, 0x00001000, 0x00100000},
};

static void* lhv_ept_gzp(void *vctx, size_t alignment, size_t sz)
{
	lhv_ept_ctx_t *ept_ctx = (lhv_ept_ctx_t *)vctx;
	u32 i;
	HALT_ON_ERRORCOND(alignment == PAGE_SIZE_4K);
	HALT_ON_ERRORCOND(sz == PAGE_SIZE_4K);
	for (i = 0; i < EPT_POOL_SIZE; i++) {
		if (!ept_ctx->page_alloc[i]) {
			ept_ctx->page_alloc[i] = 1;
			return ept_ctx->page_pool[i];
		}
	}
	return NULL;
}

static hpt_pa_t lhv_ept_ptr2pa(void *vctx, void *ptr)
{
	(void)vctx;
	return hva2spa(ptr);
}

static void* lhv_ept_pa2ptr(void *vctx, hpt_pa_t spa, size_t sz, hpt_prot_t access_type, hptw_cpl_t cpl, size_t *avail_sz)
{
	(void)vctx;
	(void)access_type;
	(void)cpl;
	*avail_sz = sz;
	return spa2hva(spa);
}

/* Get memory type from MTRR, similar to _vmx_getmemorytypeforphysicalpage() */
static u8 ept_get_mem_type(VCPU *vcpu, u64 pagebaseaddr)
{
	u32 prev_type = MTRR_TYPE_RESV;
	u32 i = 0;

	/* If MTRRs not enabled, return UC */
	if (!vcpu->vmx_ept_mtrr_enable) {
		return MTRR_TYPE_UC;
	}
	/* If fixed MTRRs are enabled, and addr < 1M, use them */
	if (pagebaseaddr < 0x100000ULL && vcpu->vmx_ept_fixmtrr_enable) {
		for (i = 0; i < NUM_FIXED_MTRRS; i++) {
			struct _fixed_mtrr_prop_t *prop = &fixed_mtrr_prop[i];
			if (pagebaseaddr < prop->end) {
				u32 index = (prop->start - pagebaseaddr) / prop->step;
				u64 msrval = vcpu->vmx_guestmtrrmsrs.fix_mtrrs[i];
				return (u8) (msrval >> (index * 8));
			}
		}
		/*
		 * Should be impossible because the last entry in fixed_mtrr_prop is 1M.
		 */
		HALT_ON_ERRORCOND(0 && "Unknown fixed MTRR");
	}
	/* Compute variable MTRRs */
	for (i = 0; i < vcpu->vmx_guestmtrrmsrs.var_count; i++) {
		u64 base = vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].base;
		u64 mask = vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].mask;
		u32 cur_type = base & 0xFFU;
		/* Check valid bit */
		if (!(mask & (1ULL << 11))) {
			continue;
		}
		/* Clear lower bits, test whether address in range */
		base &= ~0xFFFULL;
		mask &= ~0xFFFULL;
		if ((pagebaseaddr & mask) != (base & mask)) {
			continue;
		}
		/* Check for conflict resolutions: UC + * = UC; WB + WT = WT */
		if (prev_type == MTRR_TYPE_RESV || prev_type == cur_type) {
			prev_type = cur_type;
		} else if (prev_type == MTRR_TYPE_UC || cur_type == MTRR_TYPE_UC) {
			prev_type = MTRR_TYPE_UC;
		} else if (prev_type == MTRR_TYPE_WB && cur_type == MTRR_TYPE_WT) {
			prev_type = MTRR_TYPE_WT;
		} else if (prev_type == MTRR_TYPE_WT && cur_type == MTRR_TYPE_WB) {
			prev_type = MTRR_TYPE_WT;
		} else {
			printf("Conflicting MTRR types (%u, %u), HALT!\n", prev_type, cur_type);
			HALT();
		}
	}
	/* If not covered by any MTRR, use default type */
	if (prev_type == MTRR_TYPE_RESV) {
		prev_type = vcpu->vmx_ept_defaulttype;
	}
	return prev_type;
}

static void ept_map_continuous_addr(VCPU *vcpu, lhv_ept_ctx_t *ept_ctx,
									hpt_pmeo_t *pmeo, u64 low, u64 high)
{
	u64 paddr;
	printf("CPU(0x%02x): EPT 0x%08llx id-map 0x%08llx - 0x%08llx\n",
		   vcpu->id, ept_ctx->ctx.root_pa, low, high);
	for (paddr = low; paddr < high; paddr += PA_PAGE_SIZE_4K) {
		hpt_pmeo_setcache(pmeo, ept_get_mem_type(vcpu, paddr));
		hpt_pmeo_set_address(pmeo, paddr);
		HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&ept_ctx->ctx, pmeo,
												 paddr) == 0);
	}
}

void lhv_ept_init(VCPU *vcpu)
{
	/* Obtain MAXPHYADDR and compute paddrmask */
	{
		u32 eax, ebx, ecx, edx;
		/* Check whether CPUID 0x80000008 is supported */
		cpuid(0x80000000U, &eax, &ebx, &ecx, &edx);
		HALT_ON_ERRORCOND(eax >= 0x80000008U);
		/* Compute paddrmask from CPUID.80000008H:EAX[7:0] (max phy addr) */
		cpuid(0x80000008U, &eax, &ebx, &ecx, &edx);
		eax &= 0xFFU;
		HALT_ON_ERRORCOND(eax >= 32 && eax < 64);
		vcpu->vmx_ept_paddrmask = (1ULL << eax) - 0x1000ULL;
	}
	{
		u64 msrval = rdmsr64(IA32_MTRRCAP);
		vcpu->vmx_guestmtrrmsrs.var_count = (u8) msrval;
		//sanity check that fixed MTRRs are supported
		HALT_ON_ERRORCOND(msrval & (1U << 8));
		//ensure number of variable MTRRs are within the maximum supported
		HALT_ON_ERRORCOND( (vcpu->vmx_guestmtrrmsrs.var_count <=
							MAX_VARIABLE_MTRR_PAIRS) );
	}
	{
		u64 msrval = rdmsr64(IA32_MTRR_DEF_TYPE);
		vcpu->vmx_guestmtrrmsrs.def_type = msrval;
		vcpu->vmx_ept_mtrr_enable = !!(msrval & 0x800U);
		vcpu->vmx_ept_fixmtrr_enable = !!(msrval & 0x400U);
		vcpu->vmx_ept_defaulttype = msrval & 0xffU;
	}
	{
		u32 i;
		for (i = 0; i < NUM_FIXED_MTRRS; i++) {
			u64 val = rdmsr64(fixed_mtrr_prop[i].msr);
			vcpu->vmx_guestmtrrmsrs.fix_mtrrs[i] = val;
		}
	}
	{
		u32 i;
		for (i = 0; i < vcpu->vmx_guestmtrrmsrs.var_count; i++) {
			u64 baseval = rdmsr64(IA32_MTRR_PHYSBASE0 + 2 * i);
			u64 maskval = rdmsr64(IA32_MTRR_PHYSMASK0 + 2 * i);
			vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].base = baseval;
			vcpu->vmx_guestmtrrmsrs.var_mtrrs[i].mask = maskval;
		}
	}
}

u64 lhv_build_ept(VCPU *vcpu, u8 ept_num)
{
	u64 low = rpb->XtVmmRuntimePhysBase;
#ifdef __SKIP_RUNTIME_BSS__
	u64 high = rpb->XtVmmRuntimeBssEnd;
#else /* !__SKIP_RUNTIME_BSS__ */
	u64 high = low + rpb->XtVmmRuntimeSize;
#endif /* __SKIP_RUNTIME_BSS__ */
	lhv_ept_ctx_t ept_ctx;
	hpt_pmeo_t pmeo;
	/* Assuming that ept_pool and ept_alloc are initialized to 0 by bss */
	ept_ctx.ctx.gzp = lhv_ept_gzp;
	ept_ctx.ctx.pa2ptr = lhv_ept_pa2ptr;
	ept_ctx.ctx.ptr2pa = lhv_ept_ptr2pa;
	ept_ctx.ctx.root_pa = hva2spa(ept_root[vcpu->idx][ept_num >> 4]);
	ept_ctx.ctx.t = HPT_TYPE_EPT;
	ept_ctx.page_pool = ept_pool[vcpu->idx][ept_num >> 4];
	ept_ctx.page_alloc = ept_alloc[vcpu->idx][ept_num >> 4];
	pmeo.pme = 0;
	pmeo.t = HPT_TYPE_EPT;
	pmeo.lvl = 1;
	hpt_pmeo_setuser(&pmeo, true);
	hpt_pmeo_setprot(&pmeo, HPT_PROTS_RWX);
	/* hpt_pmeo_setcache() and hpt_pmeo_set_address() will be called later */
	/* Skip building most of the EPT if already built */
	if (!ept_valid[vcpu->idx][ept_num >> 4]) {
		ept_valid[vcpu->idx][ept_num >> 4] = 1;
		/* Regular memory */
		ept_map_continuous_addr(vcpu, &ept_ctx, &pmeo, low, high);
		/* LAPIC */
		ept_map_continuous_addr(vcpu, &ept_ctx, &pmeo, 0xfee00000, 0xfee01000);
		/* Console */
		ept_map_continuous_addr(vcpu, &ept_ctx, &pmeo, 0x000b8000, 0x000b9000);
		/* Real mode */
		ept_map_continuous_addr(vcpu, &ept_ctx, &pmeo, 0x00000000, 0x00100000);
	} else {
		HALT_ON_ERRORCOND(__vmx_invept(VMX_INVEPT_SINGLECONTEXT,
									   ept_ctx.ctx.root_pa | 0x1eULL));
		// HALT_ON_ERRORCOND(__vmx_invept(VMX_INVEPT_GLOBAL, 0));
	}

	/* Map 0x12340000 to ept_target */
	{
		memset(ept_target[ept_num], ept_num, 16);
		if (ept_num) {
			hpt_pmeo_setcache(&pmeo, HPT_PMT_WB);
			hpt_pmeo_set_address(&pmeo, hva2spa(ept_target[ept_num]));
		} else {
			pmeo.pme = 0;
		}
		HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&ept_ctx.ctx, &pmeo,
												 0x12340000ULL) == 0);
	}

	/* Swap large_pages using 2M pages */
	if (__LHV_OPT__ & LHV_USE_LARGE_PAGE) {
		spa_t addr0 = hva2spa(large_pages[0]);
		spa_t addr1 = hva2spa(large_pages[1]);
		HALT_ON_ERRORCOND(PA_PAGE_ALIGNED_2M(addr0));
		HALT_ON_ERRORCOND(PA_PAGE_ALIGNED_2M(addr1));
		pmeo.lvl = 2;
		pmeo.pme = 0;
		hpt_pmeo_set_page(&pmeo, true);
		hpt_pmeo_setuser(&pmeo, true);
		hpt_pmeo_setprot(&pmeo, HPT_PROTS_RWX);
		hpt_pmeo_setcache(&pmeo, HPT_PMT_WB);
		/* lage_pages[1] -> lage_pages[0] */
		hpt_pmeo_set_address(&pmeo, addr0);
		HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&ept_ctx.ctx, &pmeo,
												 addr1) == 0);
		/* lage_pages[1] -> lage_pages[1] */
		hpt_pmeo_set_address(&pmeo, addr1);
		HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&ept_ctx.ctx, &pmeo,
												 addr0) == 0);
		memset(large_pages[0], 'A', 16);
		memset(large_pages[1], 'B', 16);
	}

	return ept_ctx.ctx.root_pa;
}

