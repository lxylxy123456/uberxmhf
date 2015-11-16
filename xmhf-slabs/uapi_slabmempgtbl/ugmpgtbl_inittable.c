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

/*
 * slab memory pagetable uAPI
 *
 * author: amit vasudevan (amitvasudevan@acm.org)
 */

#include <xmhf.h>
#include <xmhf-debug.h>

#include <xmhfgeec.h>

//#include <xc.h>
//#include <geec_sentinel.h>
#include <uapi_slabmempgtbl.h>



/*@
	requires 0 <= slabid < XMHFGEEC_TOTAL_UGSLABS;
	assigns _slabmempgtbl_lvl4t[slabid][0..(PAE_MAXPTRS_PER_PML4T-1)];
	assigns _slabmempgtbl_lvl3t[slabid][0..(PAE_MAXPTRS_PER_PDPT-1)];
	ensures (\forall u32 x; 0 <= x < PAE_PTRS_PER_PML4T ==>
		 ( (u64)_slabmempgtbl_lvl4t[slabid][x] ) == ( ((u64)&_slabmempgtbl_lvl3t[slabid] | 0x7) )
		);
@*/
void _slabmempgtbl_initmempgtbl_ept4K(u32 slabid){
	u32 i, j;

	//pml4t zero out
	/*@
		loop invariant a1: 0 <= i <= PAE_MAXPTRS_PER_PML4T;
		loop invariant a2: \forall integer x; 0 <= x < i ==> ( (u64)_slabmempgtbl_lvl4t[slabid][x] == 0 );
		loop assigns _slabmempgtbl_lvl4t[slabid][0..(PAE_MAXPTRS_PER_PML4T-1)];
		loop assigns i;
		loop variant PAE_MAXPTRS_PER_PML4T - i;
	@*/
	for(i=0; i < PAE_MAXPTRS_PER_PML4T; i++){
		_slabmempgtbl_lvl4t[slabid][i] = 0;
	}
	//memset(&_slabmempgtbl_lvl4t[slabid], 0, PAGE_SIZE_4K);

	//pdpt zero out
	//memset(&_slabmempgtbl_lvl3t[slabid], 0, PAGE_SIZE_4K);
	/*@
		loop invariant a1: 0 <= i <= PAE_MAXPTRS_PER_PDPT;
		loop invariant a2: \forall integer x; 0 <= x < i ==> ( (u64)_slabmempgtbl_lvl3t[slabid][x] == 0 );
		loop assigns _slabmempgtbl_lvl3t[slabid][0..(PAE_MAXPTRS_PER_PDPT-1)];
		loop assigns i;
		loop variant PAE_MAXPTRS_PER_PDPT - i;
	@*/
	for(i=0; i < PAE_MAXPTRS_PER_PDPT; i++){
		_slabmempgtbl_lvl3t[slabid][i] = 0;
	}


	//pml4t setup
	/*@
		loop invariant a1: 0 <= i <= PAE_PTRS_PER_PML4T;
		loop invariant a2: \forall integer x; 0 <= x < i ==> ( (u64)_slabmempgtbl_lvl4t[slabid][x] == ((u64)&_slabmempgtbl_lvl3t[slabid] | 0x7) );
		loop assigns _slabmempgtbl_lvl4t[slabid][0..(PAE_PTRS_PER_PML4T-1)];
		loop assigns i;
		loop variant PAE_PTRS_PER_PML4T - i;
	@*/
	for(i=0; i < PAE_PTRS_PER_PML4T; i++){
		_slabmempgtbl_lvl4t[slabid][i] = ((u64)&_slabmempgtbl_lvl3t[slabid] | 0x7);
	}


#if 0

	for(i=0; i < PAE_PTRS_PER_PDPT; i++){
		_slabmempgtbl_lvl3t[slabid][i] = ((u64)&_slabmempgtbl_lvl2t[slabid][i] | 0x7 );
	}


    //pdt
	for(i=0; i < PAE_PTRS_PER_PDPT; i++){
		for(j=0; j < PAE_PTRS_PER_PDT; j++){
			_slabmempgtbl_lvl2t[slabid][i][j] =
                ((u64)&_slabmempgtbl_lvl1t[slabid][i][j] | 0x7 );
		}
	}
#endif
}



#if 0
//@ghost bool inittable_invokeept4K=false;
/*@
	requires \valid(initmempgtblp);

	behavior inittable_do:
		assumes (
			 (initmempgtblp->dst_slabid < XMHFGEEC_TOTAL_SLABS) &&
			(initmempgtblp->dst_slabid >= XMHFGEEC_UGSLAB_BASE_IDX && initmempgtblp->dst_slabid <= XMHFGEEC_UGSLAB_MAX_IDX) &&
			(xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVT_PROG_GUEST ||
			 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_GUEST ||
			 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_RICHGUEST)
			);
		assigns inittable_invokeept4K;
		ensures (inittable_invokeept4K == true);

	behavior inittable_invalid:
		assumes !(
			 (initmempgtblp->dst_slabid < XMHFGEEC_TOTAL_SLABS) &&
			(initmempgtblp->dst_slabid >= XMHFGEEC_UGSLAB_BASE_IDX && initmempgtblp->dst_slabid <= XMHFGEEC_UGSLAB_MAX_IDX) &&
			(xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVT_PROG_GUEST ||
			 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_GUEST ||
			 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_RICHGUEST)
			);

		assigns inittable_invokeept4K;
		ensures (inittable_invokeept4K == false);

	complete behaviors;
	disjoint behaviors;

@*/
void _slabmempgtbl_initmempgtbl(xmhfgeec_uapi_slabmempgtbl_initmempgtbl_params_t *initmempgtblp){

    if( (initmempgtblp->dst_slabid < XMHFGEEC_TOTAL_SLABS) &&
	(initmempgtblp->dst_slabid >= XMHFGEEC_UGSLAB_BASE_IDX && initmempgtblp->dst_slabid <= XMHFGEEC_UGSLAB_MAX_IDX) &&
	(xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVT_PROG_GUEST ||
	 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_GUEST ||
	 xmhfgeec_slab_info_table[initmempgtblp->dst_slabid].slabtype == XMHFGEEC_SLABTYPE_uVU_PROG_RICHGUEST)
      ){

            //_slabmempgtbl_initmempgtbl_ept4K((initmempgtblp->dst_slabid - XMHFGEEC_UGSLAB_BASE_IDX));
	//@ghost inittable_invokeept4K = true;

            _XDPRINTF_("%s: setup slab %u with ept4K\n", __func__, slabid);

	}else{
		//_XDPRINTF_("%s: Halting. Unknown slab type %u\n", __func__, slabtype);
		//@ghost inittable_invokeept4K = false;
	}
}
#endif // 0

