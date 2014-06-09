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
 * 
 *  XMHF core API 
 * 
 *  author: amit vasudevan (amitvasudevan@acm.org)
 */

#ifndef __XC_COREAPI_H__
#define __XC_COREAPI_H__

//core APIs
#define	XC_API_HPT_SETPROT							(0xA01)
#define	XC_API_HPT_GETPROT							(0xA02)
#define XC_API_HPT_SETENTRY							(0xA03)
#define XC_API_HPT_GETENTRY							(0xA04)
#define XC_API_HPT_FLUSHCACHES						(0xA05)
#define XC_API_HPT_FLUSHCACHES_SMP					(0xA06)
#define XC_API_HPT_LVL2PAGEWALK						(0xA07)


#define	XC_API_TRAPMASK_SET							(0xB01)
#define XC_API_TRAPMASK_CLEAR						(0xB02)

#define XC_API_CPUSTATE_SET							(0xC01)
#define XC_API_CPUSTATE_GET							(0xC02)


// memory protection types
#define MEMP_PROT_NOTPRESENT	(1)	// page not present
#define	MEMP_PROT_PRESENT		(2)	// page present
#define MEMP_PROT_READONLY		(4)	// page read-only
#define MEMP_PROT_READWRITE		(8) // page read-write
#define MEMP_PROT_EXECUTE		(16) // page execute
#define MEMP_PROT_NOEXECUTE		(32) // page no-execute
#define MEMP_PROT_MAXVALUE		(MEMP_PROT_NOTPRESENT+MEMP_PROT_PRESENT+MEMP_PROT_READONLY+MEMP_PROT_READWRITE+MEMP_PROT_NOEXECUTE+MEMP_PROT_EXECUTE)


#define XMHF_SLAB_COREAPI_FNXCAPIHPTSETPROT						0				
#define XMHF_SLAB_COREAPI_FNXCAPIHPTGETPROT						1
#define XMHF_SLAB_COREAPI_FNXCAPIHPTSETENTRY					2	
#define XMHF_SLAB_COREAPI_FNXCAPIHPTGETENTRY					3	
#define XMHF_SLAB_COREAPI_FNXCAPIHPTFLUSHCACHES					4
#define XMHF_SLAB_COREAPI_FNXCAPIHPTFLUSHCACHESSMP				5	
#define XMHF_SLAB_COREAPI_FNXCAPIHPTLVL2PAGEWALK				6	

#define XMHF_SLAB_COREAPI_FNXCAPITRAPMASKSET					7	
#define XMHF_SLAB_COREAPI_FNXCAPITRAPMASKCLEAR					8	

#define XMHF_SLAB_COREAPI_FNXCAPICPUSTATESET					9	
#define XMHF_SLAB_COREAPI_FNXCAPICPUSTATEGET					10	

#define XMHF_SLAB_COREAPI_FNXCAPIPARTITIONCREATE				11	
#define XMHF_SLAB_COREAPI_FNXCAPIPARTITIONADDCPU				12	
#define XMHF_SLAB_COREAPI_FNXCAPIPARTITIONGETCONTEXTDESC		13	
#define XMHF_SLAB_COREAPI_FNXCAPIPARTITIONSTARTCPU				14	

#define XMHF_SLAB_COREAPI_FNXCAPIPLATFORMSHUTDOWN				15	

#define	XMHF_SLAB_COREAPI_FNXCCOREAPIARCHEVENTHANDLERNMIEXCEPTION	16

#ifndef __ASSEMBLY__


#ifdef __XMHF_SLAB_INTERNAL_USE__


//HPT related core APIs
void xc_api_hpt_setprot(context_desc_t context_desc, u64 gpa, u32 prottype);
void xc_api_hpt_arch_setprot(context_desc_t context_desc, u64 gpa, u32 prottype);

u32 xc_api_hpt_getprot(context_desc_t context_desc, u64 gpa);
u32 xc_api_hpt_arch_getprot(context_desc_t context_desc, u64 gpa);

void xc_api_hpt_setentry(context_desc_t context_desc, u64 gpa, u64 entry);
void xc_api_hpt_arch_setentry(context_desc_t context_desc, u64 gpa, u64 entry);

u64 xc_api_hpt_getentry(context_desc_t context_desc, u64 gpa);
u64 xc_api_hpt_arch_getentry(context_desc_t context_desc, u64 gpa);

void xc_api_hpt_flushcaches(context_desc_t context_desc);
void xc_api_hpt_flushcaches_smp(context_desc_t context_desc);
void xc_api_hpt_arch_flushcaches(context_desc_t context_desc, bool dosmpflush);

u64 xc_api_hpt_lvl2pagewalk(context_desc_t context_desc, u64 gva);
u64 xc_api_hpt_arch_lvl2pagewalk(context_desc_t context_desc, u64 gva);


//trapmask related core APIs
void xc_api_trapmask_set(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams);
void xc_api_trapmask_arch_set(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams);

void xc_api_trapmask_clear(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams);
void xc_api_trapmask_arch_clear(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams);


//cpu state related core APIs
void xc_api_cpustate_set(context_desc_t context_desc, xc_hypapp_arch_param_t cpustateparams);
void xc_api_cpustate_arch_set(context_desc_t context_desc, xc_hypapp_arch_param_t cpustateparams);

xc_hypapp_arch_param_t xc_api_cpustate_get(context_desc_t context_desc, u64 operation);
xc_hypapp_arch_param_t xc_api_cpustate_arch_get(context_desc_t context_desc, u64 operation);


//partition related core APIs
u32 xc_api_partition_create(u32 partitiontype);	//returns partition_index
context_desc_t xc_api_partition_addcpu(u32 partition_index, u32 cpuid, bool is_bsp); //return cpu_index
context_desc_t xc_api_partition_getcontextdesc(u32 cpuid); //return context_desc_t structure
bool xc_api_partition_startcpu(context_desc_t context_desc);
bool xc_api_partition_arch_startcpu(context_desc_t context_desc);


bool xc_api_partition_arch_addcpu(u32 partition_index, u32 cpu_index);


//platform related core APIs
void xc_api_platform_shutdown(context_desc_t context_desc);
void xc_api_platform_arch_shutdown(context_desc_t context_desc);

//global data

// platform cpus
extern xc_cpu_t g_xc_cpu[MAX_PLATFORM_CPUS] __attribute__(( section(".data") ));

// primary partitions
extern xc_partition_t g_xc_primary_partition[MAX_PRIMARY_PARTITIONS] __attribute__(( section(".data") ));

//core API NMI callback
void xc_coreapi_arch_eventhandler_nmiexception(struct regs *r);


#else  //!__XMHF_SLAB_INTERNAL_USE__
extern slab_header_t _slab_table[];

XMHF_SLAB_DEFIMPORTFN(void xc_api_hpt_setprot(context_desc_t context_desc, u64 gpa, u32 prottype), 						XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTSETPROT					, (sizeof(context_desc_t)+sizeof(u64)+sizeof(u32))			, 0)								)
XMHF_SLAB_DEFIMPORTFN(u32 xc_api_hpt_getprot(context_desc_t context_desc, u64 gpa), 									XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTGETPROT					, (sizeof(context_desc_t)+sizeof(u64))						, 0)								)
XMHF_SLAB_DEFIMPORTFN(void xc_api_hpt_setentry(context_desc_t context_desc, u64 gpa, u64 entry), 						XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTSETENTRY				, (sizeof(context_desc_t)+sizeof(u64)+sizeof(u64))			, 0)								)
XMHF_SLAB_DEFIMPORTFN(u64 xc_api_hpt_getentry(context_desc_t context_desc, u64 gpa), 									XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTGETENTRY				, (sizeof(context_desc_t)+sizeof(u64))						, 0)								)
XMHF_SLAB_DEFIMPORTFN(void xc_api_hpt_flushcaches(context_desc_t context_desc), 										XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTFLUSHCACHES				, (sizeof(context_desc_t))									, 0)								)
XMHF_SLAB_DEFIMPORTFN(void xc_api_hpt_flushcaches_smp(context_desc_t context_desc), 									XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTFLUSHCACHESSMP			, (sizeof(context_desc_t))									, 0)								)
XMHF_SLAB_DEFIMPORTFN(u64 xc_api_hpt_lvl2pagewalk(context_desc_t context_desc, u64 gva), 								XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIHPTLVL2PAGEWALK			, (sizeof(context_desc_t)+sizeof(u64))						, 0)								)
                                                                            
XMHF_SLAB_DEFIMPORTFN(void xc_api_trapmask_set(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams), 	XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPITRAPMASKSET				, (sizeof(context_desc_t)+sizeof(xc_hypapp_arch_param_t))	, 0)								)
XMHF_SLAB_DEFIMPORTFN(void xc_api_trapmask_clear(context_desc_t context_desc, xc_hypapp_arch_param_t trapmaskparams), 	XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPITRAPMASKCLEAR				, (sizeof(context_desc_t)+sizeof(xc_hypapp_arch_param_t))	, 0)								)
                                                                            
XMHF_SLAB_DEFIMPORTFN(void xc_api_cpustate_set(context_desc_t context_desc, xc_hypapp_arch_param_t cpustateparams), 	XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPICPUSTATESET				, (sizeof(context_desc_t)+sizeof(xc_hypapp_arch_param_t))	, 0)								)
XMHF_SLAB_DEFIMPORTFN(xc_hypapp_arch_param_t xc_api_cpustate_get(context_desc_t context_desc, u64 operation), 			XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPICPUSTATEGET				, (sizeof(context_desc_t)+sizeof(u64)+sizeof(u32))			, sizeof(xc_hypapp_arch_param_t))	)
                                                                            
XMHF_SLAB_DEFIMPORTFN(u32 xc_api_partition_create(u32 partitiontype),													XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIPARTITIONCREATE			, (sizeof(u32))												, 0)								)
XMHF_SLAB_DEFIMPORTFN(context_desc_t xc_api_partition_addcpu(u32 partition_index, u32 cpuid, bool is_bsp), 				XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIPARTITIONADDCPU			, (sizeof(u32)+sizeof(u32)+sizeof(bool)+sizeof(u32))		, sizeof(context_desc_t))			)
XMHF_SLAB_DEFIMPORTFN(context_desc_t xc_api_partition_getcontextdesc(u32 cpuid), 										XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIPARTITIONGETCONTEXTDESC	, (sizeof(u32)+sizeof(u32))									, sizeof(context_desc_t))			)
XMHF_SLAB_DEFIMPORTFN(bool xc_api_partition_startcpu(context_desc_t context_desc), 										XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIPARTITIONSTARTCPU			, (sizeof(context_desc_t))									, 0)								)
                                                                            
XMHF_SLAB_DEFIMPORTFN(void xc_api_platform_shutdown(context_desc_t context_desc),										XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX,	XMHF_SLAB_COREAPI_FNXCAPIPLATFORMSHUTDOWN			, (sizeof(context_desc_t))									, 0)								)

XMHF_SLAB_DEFIMPORTFN(void xc_coreapi_arch_eventhandler_nmiexception(struct regs *r),									XMHF_SLAB_DEFIMPORTFNSTUB(XMHF_SLAB_COREAPI_INDEX, 	XMHF_SLAB_COREAPI_FNXCCOREAPIARCHEVENTHANDLERNMIEXCEPTION, (sizeof(struct regs *)), 0)	)
#endif //__XMHF_SLAB_INTERNAL_USE__


#endif	//__ASSEMBLY__


#endif //__XC_COREAPI_H__
