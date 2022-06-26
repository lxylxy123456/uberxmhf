#include <xmhf.h>
#include <lhv.h>

#define EPT_POOL_SIZE 128

/* Root EPT pages */
static u8 ept_root[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

/* Physical memory for storing more  */
static u8 ept_pool[MAX_VCPU_ENTRIES][EPT_POOL_SIZE][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

/* Indicate whether the page in ept_pool is free */
static u8 ept_alloc[MAX_VCPU_ENTRIES][EPT_POOL_SIZE];

typedef struct {
	hptw_ctx_t ctx;
	u8 (*page_pool)[PAGE_SIZE_4K];
	u8 *page_alloc;
} lhv_ept_ctx_t;

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

u64 lhv_build_ept(VCPU *vcpu)
{
	u64 low = rpb->XtVmmRuntimePhysBase;
	u64 high = low + rpb->XtVmmRuntimeSize;
	lhv_ept_ctx_t ept_ctx;
	hpt_pmeo_t pmeo;
	ept_ctx.ctx.gzp = lhv_ept_gzp;
	ept_ctx.ctx.pa2ptr = lhv_ept_pa2ptr;
	ept_ctx.ctx.ptr2pa = lhv_ept_ptr2pa;
	ept_ctx.ctx.root_pa = hva2spa(ept_root[vcpu->idx]);
	ept_ctx.ctx.t = HPT_TYPE_EPT;
	ept_ctx.page_pool = ept_pool[vcpu->idx];
	ept_ctx.page_alloc = ept_alloc[vcpu->idx];
	pmeo.pme = 0;
	pmeo.t = HPT_TYPE_EPT;
	pmeo.lvl = 1;
	// TODO
//	hpt_pmeo_setuser(&pmeo, true);
//	hpt_pmeo_setprot(&pmeo, HPT_PROTS_RWX);
//	hpt_pmeo_set_address(&pmeo, 0x01234000);
	/* Assuming that ept_pool and ept_alloc are initialized to 0 by bss */
	for (int i = 0; i >= 0; i += 10) {}
	HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&ept_ctx.ctx, &pmeo, 0x01234000) == 0);
	HALT_ON_ERRORCOND(0 && "TODO frontier");
	(void) low;
	(void) high;
	(void) vcpu;
	HALT_ON_ERRORCOND((void *)hptw_insert_pmeo != (void *)hptw_get_pmo_alloc);
	return 0;
	return 0xdead0000;
}

