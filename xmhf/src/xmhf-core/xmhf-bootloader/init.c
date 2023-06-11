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

//init.c - EMHF early initialization blob functionality
//author: amit vasudevan (amitvasudevan@acm.org)

//---includes-------------------------------------------------------------------
#include <xmhf.h>

//---forward prototypes---------------------------------------------------------
u32 smp_getinfo(PCPU *pcpus, u32 *num_pcpus, void *uefi_rsdp);
MPFP * MP_GetFPStructure(void);
u32 _MPFPComputeChecksum(u32 spaddr, u32 size);
u32 isbsp(void);

//---globals--------------------------------------------------------------------
PCPU pcpus[MAX_PCPU_ENTRIES];
u32 pcpus_numentries=0;
u32 cpu_vendor;    //CPU_VENDOR_INTEL or CPU_VENDOR_AMD
uintptr_t hypervisor_image_baseaddress;    //2M aligned highest physical memory address
//where the hypervisor binary is relocated to
GRUBE820 grube820list[MAX_E820_ENTRIES];
u32 grube820list_numentries=0;        //actual number of e820 entries returned
//by grub

//master-id table which holds LAPIC ID to VCPU mapping for each physical core
MIDTAB midtable[MAX_MIDTAB_ENTRIES] __attribute__(( section(".data") ));

//number of physical cores in the system
u32 midtable_numentries=0;

//VCPU buffers for all cores
VCPU vcpubuffers[MAX_VCPU_ENTRIES] __attribute__(( section(".data") ));

//initial stacks for all cores
u8 cpustacks[RUNTIME_STACK_SIZE * MAX_PCPU_ENTRIES] __attribute__(( section(".stack") ));

SL_PARAMETER_BLOCK *slpb = NULL;

extern void init_core_lowlevel_setup(void);

/* Don't break the build if the Makefile fails to define these. */
#ifndef ___RUNTIME_INTEGRITY_HASH___
#define ___RUNTIME_INTEGRITY_HASH___ BAD_INTEGRITY_HASH
#endif /*  ___RUNTIME_INTEGRITY_HASH___ */
#ifndef ___SLABOVE64K_INTEGRITY_HASH___
#define ___SLABOVE64K_INTEGRITY_HASH___ BAD_INTEGRITY_HASH
#endif /*  ___SLABOVE64K_INTEGRITY_HASH___ */
#ifndef ___SLBELOW64K_INTEGRITY_HASH___
#define ___SLBELOW64K_INTEGRITY_HASH___ BAD_INTEGRITY_HASH
#endif /*  ___SLBELOW64K_INTEGRITY_HASH___ */

// we should get all of these from the build process, but don't forget
// that here in 'init' these values are UNTRUSTED
INTEGRITY_MEASUREMENT_VALUES g_init_gold /* __attribute__(( section("") )) */ = {
    .sha_runtime = ___RUNTIME_INTEGRITY_HASH___,
    .sha_slabove64K = ___SLABOVE64K_INTEGRITY_HASH___,
    .sha_slbelow64K = ___SLBELOW64K_INTEGRITY_HASH___
};

//size of SL + runtime in bytes
size_t sl_rt_size;


//---MP config table handling---------------------------------------------------
void dealwithMP(void *uefi_rsdp){
    if(!smp_getinfo(pcpus, &pcpus_numentries, uefi_rsdp)){
        printf("Fatal error with SMP detection. Halting!\n");
        HALT();
    }
}

//---INIT IPI routine-----------------------------------------------------------
void send_init_ipi_to_all_APs(void) {
    u32 eax, edx;
    volatile u32 *icr;
    u32 timeout = 0x01000000;

    //read LAPIC base address from MSR
    rdmsr(MSR_APIC_BASE, &eax, &edx);
    HALT_ON_ERRORCOND( edx == 0 ); //APIC is below 4G
    printf("LAPIC base and status=0x%08x\n", eax);

    icr = (u32 *) (((u32)eax & 0xFFFFF000UL) + 0x300);

    //send INIT
    printf("Sending INIT IPI to all APs...\n");
    *icr = 0x000c4500UL;
    xmhf_baseplatform_arch_x86_udelay(10000);
    //wait for command completion
    while (--timeout > 0 && ((*icr) & 0x00001000U)) {
        xmhf_cpu_relax();
    }
    if(timeout == 0) {
        printf("\nERROR: send_init_ipi_to_all_APs() TIMEOUT!\n");
    }
    printf("\nDone.\n");
}



#ifndef __UEFI__
//---E820 parsing and handling--------------------------------------------------
//runtimesize is assumed to be 2M aligned
u32 dealwithE820(multiboot_info_t *mbi, u32 runtimesize __attribute__((unused))){
    //check if GRUB has a valid E820 map
    if(!(mbi->flags & MBI_MEMMAP)){
        printf("%s: no E820 map provided. HALT!\n", __FUNCTION__);
        HALT();
    }

    //zero out grub e820 list
    memset((void *)&grube820list, 0, sizeof(GRUBE820)*MAX_E820_ENTRIES);

    //grab e820 list into grube820list
    {
        // TODO: grube820list_numentries < MAX_E820_ENTRIES not checked.
        // Possible buffer overflow?
        memory_map_t *mmap;
        for ( mmap = (memory_map_t *) mbi->mmap_addr;
              (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
              mmap = (memory_map_t *) ((unsigned long) mmap
                                       + mmap->size + sizeof (mmap->size))){
            grube820list[grube820list_numentries].baseaddr_low = mmap->base_addr_low;
            grube820list[grube820list_numentries].baseaddr_high = mmap->base_addr_high;
            grube820list[grube820list_numentries].length_low = mmap->length_low;
            grube820list[grube820list_numentries].length_high = mmap->length_high;
            grube820list[grube820list_numentries].type = mmap->type;
            grube820list_numentries++;
        }
    }

    //debug: print grube820list
    {
        u32 i;
        printf("\noriginal system E820 map follows:\n");
        for(i=0; i < grube820list_numentries; i++){
            printf("0x%08x%08x, size=0x%08x%08x (%u)\n",
                   grube820list[i].baseaddr_high, grube820list[i].baseaddr_low,
                   grube820list[i].length_high, grube820list[i].length_low,
                   grube820list[i].type);
        }

    }

    //traverse e820 list forward to find an entry with type=0x1 (free)
    //with free amount of memory for runtime
    {
        u32 foundentry=0;
        u32 slruntimephysicalbase=__TARGET_BASE_SL;	//SL + runtime base
        u32 i;

        //for(i= (int)(grube820list_numentries-1); i >=0; i--){
		for(i= 0; i < grube820list_numentries; i++){
            u32 baseaddr, size;
            baseaddr = grube820list[i].baseaddr_low;
            size = grube820list[i].length_low;

            if(grube820list[i].type == 0x1){ //free memory?
                if(grube820list[i].baseaddr_high) //greater than 4GB? then skip
                    continue;

                if(grube820list[i].length_high){
                    printf("%s: E820 parsing error (64-bit length for < 4GB). HALT!\n");
                    HALT();
                }

			 	//check if this E820 range can accomodate SL + runtime
			 	if( slruntimephysicalbase >= baseaddr && (slruntimephysicalbase + runtimesize) < (baseaddr + size) ){
                    foundentry=1;
                    break;
                }
            }
        }

        if(!foundentry){
            printf("%s: unable to find E820 memory for SL+runtime. HALT!\n");
            HALT();
        }

		//entry number we need to split is indexed by i
		printf("proceeding to revise E820...\n");

		{

				//temporary E820 table with index
				GRUBE820 te820[MAX_E820_ENTRIES];
				u32 j=0;

				//copy all entries from original E820 table until index i
				for(j=0; j < i; j++)
					memcpy((void *)&te820[j], (void *)&grube820list[j], sizeof(GRUBE820));

				//we need a maximum of 2 extra entries for the final table, make a sanity check
				HALT_ON_ERRORCOND( (grube820list_numentries+2) < MAX_E820_ENTRIES );

				//split entry i into required number of entries depending on the memory range alignments
				if( (slruntimephysicalbase == grube820list[i].baseaddr_low) && ((slruntimephysicalbase+runtimesize) == (grube820list[i].baseaddr_low+grube820list[i].length_low)) ){
						//exact match, no split
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=grube820list[i].baseaddr_low; te820[j].length_low=grube820list[i].length_low; te820[j].type=grube820list[i].type;
						j++;
						i++;
				}else if ( (slruntimephysicalbase == grube820list[i].baseaddr_low) && (runtimesize < grube820list[i].length_low) ){
						//left aligned, split into 2
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=grube820list[i].baseaddr_low; te820[j].length_low=runtimesize; te820[j].type=0x2;
						j++;
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=grube820list[i].baseaddr_low+runtimesize; te820[j].length_low=grube820list[i].length_low-runtimesize; te820[j].type=1;
						j++;
						i++;
				}else if ( ((slruntimephysicalbase+runtimesize) == (grube820list[i].baseaddr_low+grube820list[i].length_low)) && slruntimephysicalbase > grube820list[i].baseaddr_low ){
						//right aligned, split into 2
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=grube820list[i].baseaddr_low; te820[j].length_low=slruntimephysicalbase-grube820list[i].baseaddr_low; te820[j].type=0x1;
						j++;
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low= slruntimephysicalbase; te820[j].length_low=runtimesize; te820[j].type=0x1;
						j++;
						i++;
				}else{
						//range in the middle, split into 3
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=grube820list[i].baseaddr_low; te820[j].length_low=slruntimephysicalbase-grube820list[i].baseaddr_low; te820[j].type=0x1;
						j++;
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=slruntimephysicalbase; te820[j].length_low=runtimesize; te820[j].type=0x2;
						j++;
						te820[j].baseaddr_high=0; te820[j].length_high=0; te820[j].baseaddr_low=slruntimephysicalbase+runtimesize; te820[j].length_low=grube820list[i].length_low-runtimesize-(slruntimephysicalbase-grube820list[i].baseaddr_low); te820[j].type=1;
						j++;
						i++;
				}

				//copy entries i through end of original E820 list into temporary E820 list starting at index j
				while(i < grube820list_numentries){
					memcpy((void *)&te820[j], (void *)&grube820list[i], sizeof(GRUBE820));
					i++;
					j++;
				}

				//copy temporary E820 list into global E20 list and setup final E820 entry count
				grube820list_numentries = j;
				memcpy((void *)&grube820list, (void *)&te820, (grube820list_numentries * sizeof(GRUBE820)) );
		}

		printf("E820 revision complete.\n");

		//debug: print grube820list
		{
			u32 i;
			printf("\nrevised system E820 map follows:\n");
			for(i=0; i < grube820list_numentries; i++){
				printf("0x%08x%08x, size=0x%08x%08x (%u)\n",
					   grube820list[i].baseaddr_high, grube820list[i].baseaddr_low,
					   grube820list[i].length_high, grube820list[i].length_low,
					   grube820list[i].type);
			}
		}


        return slruntimephysicalbase;
    }

}
#endif /* !__UEFI__ */

void setupvcpus(u32 cpu_vendor, MIDTAB *midtable, u32 midtable_numentries){
    u32 i;
    VCPU *vcpu;

    printf("%s: cpustacks range 0x%08lx-0x%08lx in 0x%08lx chunks\n",
           __FUNCTION__, (uintptr_t)cpustacks,
           (uintptr_t)cpustacks + (RUNTIME_STACK_SIZE * MAX_VCPU_ENTRIES),
           (size_t)RUNTIME_STACK_SIZE);
    printf("%s: vcpubuffers range 0x%08lx-0x%08lx in 0x%08lx chunks\n",
           __FUNCTION__, (uintptr_t)vcpubuffers,
           (uintptr_t)vcpubuffers + (SIZE_STRUCT_VCPU * MAX_VCPU_ENTRIES),
           (size_t)SIZE_STRUCT_VCPU);

    for(i=0; i < midtable_numentries; i++){
        hva_t esp;
        vcpu = (VCPU *)((uintptr_t)vcpubuffers + (i * SIZE_STRUCT_VCPU));
        memset((void *)vcpu, 0, sizeof(VCPU));

        vcpu->cpu_vendor = cpu_vendor;

        esp = ((uintptr_t)cpustacks + (i * RUNTIME_STACK_SIZE)) + RUNTIME_STACK_SIZE;
#ifdef __I386__
        vcpu->esp = esp;
#elif defined(__AMD64__)
        vcpu->rsp = esp;
#else
    #error "Unsupported Arch"
#endif /* __I386__ */
        vcpu->id = midtable[i].cpu_lapic_id;

        midtable[i].vcpu_vaddr_ptr = (uintptr_t)vcpu;
        printf("CPU #%u: vcpu_vaddr_ptr=0x%08lx, esp=0x%08lx\n", i,
               (uintptr_t)midtable[i].vcpu_vaddr_ptr,
               (uintptr_t)esp);
    }
}


//---wakeupAPs------------------------------------------------------------------
void wakeupAPs(void){
    u32 eax, edx;
    volatile u32 *icr;

    //read LAPIC base address from MSR
    rdmsr(MSR_APIC_BASE, &eax, &edx);
    HALT_ON_ERRORCOND( edx == 0 ); //APIC is below 4G
    //printf("LAPIC base and status=0x%08x\n", eax);

    icr = (u32 *) (((u32)eax & 0xFFFFF000UL) + 0x300);

    {
        extern u32 _ap_bootstrap_start[], _ap_bootstrap_end[];
        memcpy((void *)0x10000, (void *)_ap_bootstrap_start, (uintptr_t)_ap_bootstrap_end - (uintptr_t)_ap_bootstrap_start + 1);
    }

#ifdef __UEFI__
    HALT_ON_ERRORCOND(0 && "TODO");
    // See __UEFI__ in initsup.S
    // Probably better to split to i386 (non-UEFI) and amd64 (UEFI) versions.
#endif /* __UEFI__ */

    //our test code is at 1000:0000, we need to send 10 as vector
    //send INIT
    printf("Sending INIT IPI to all APs...");
    *icr = 0x000c4500UL;
    xmhf_baseplatform_arch_x86_udelay(10000);
    //wait for command completion
    while ((*icr) & 0x1000U) {
        xmhf_cpu_relax();
    }
    printf("Done.\n");

    //send SIPI (twice as per the MP protocol)
    {
        int i;
        for(i=0; i < 2; i++){
            printf("Sending SIPI-%u...", i);
            *icr = 0x000c4610UL;
            xmhf_baseplatform_arch_x86_udelay(200);
            //wait for command completion
            while ((*icr) & 0x1000U) {
                xmhf_cpu_relax();
            }
            printf("Done.\n");
        }
    }

    printf("APs should be awake!\n");
}

#if 0 /* __NOT_RUNNING_LHV__ */
/* The TPM must be ready for the AMD CPU to send it commands at
 * Locality 4 when executing SKINIT. Ideally all that is necessary is
 * to xmhf_tpm_deactivate_all_localities(), but some TPM's are still not
 * sufficiently "awake" after that.  Thus, make sure it successfully
 * responds to a command at some locality, *then*
 * xmhf_tpm_deactivate_all_localities().
 */
bool svm_prepare_tpm(void) {
    uint32_t locality = EMHF_TPM_LOCALITY_PREF; /* target.h */
    bool ret = true;

    printf("INIT:TPM: prepare_tpm starting.\n");
    //dump_locality_access_regs();
    xmhf_tpm_deactivate_all_localities();
    //dump_locality_access_regs();

    if(tpm_wait_cmd_ready(locality)) {
        printf("INIT:TPM: successfully opened in Locality %d.\n", locality);
    } else {
        printf("INIT:TPM: ERROR: Locality %d could not be opened.\n", locality);
        ret = false;
    }
    xmhf_tpm_deactivate_all_localities();
    //dump_locality_access_regs();
    printf("INIT:TPM: prepare_tpm done.\n");

    return ret;
}
#endif

//---init main----------------------------------------------------------------
#ifdef __UEFI__
void cstartup(xmhf_efi_info_t *xei)
#else /* !__UEFI__ */
void cstartup(multiboot_info_t *mbi)
#endif /* __UEFI__ */
{
#ifndef __UEFI__
    module_t *mod_array;
    u32 mods_count;
    size_t sl_rt_nonzero_size;
#endif /* !__UEFI__ */

    /* parse command line */
    memset(g_cmdline, '\0', sizeof(g_cmdline));
#ifdef __UEFI__
    strncpy(g_cmdline, xei->cmdline, sizeof(g_cmdline)-1);
#else /* !__UEFI__ */
    strncpy(g_cmdline, (char*)mbi->cmdline, sizeof(g_cmdline)-1);
#endif /* __UEFI__ */
    g_cmdline[sizeof(g_cmdline)-1] = '\0'; /* in case strncpy truncated */
    tboot_parse_cmdline();

#ifndef __UEFI__
    mod_array = (module_t*)mbi->mods_addr;
    mods_count = mbi->mods_count;
#endif /* !__UEFI__ */

	//welcome banner
	printf("Lightweight Hypervisor (LHV) %s\n", ___XMHF_BUILD_VERSION___);
	printf("Build revision: %s\n", ___XMHF_BUILD_REVISION___);
	printf("LHV_OPT = 0x%016llx\n", (u64) __LHV_OPT__);
#ifdef __XMHF_AMD64__
	printf("Subarch: amd64\n\n");
#elif __XMHF_I386__
	printf("Subarch: i386\n\n");
#else /* !defined(__XMHF_I386__) && !defined(__XMHF_AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__XMHF_I386__) && !defined(__XMHF_AMD64__) */

#ifdef __UEFI__
    printf("Boot method: UEFI\n");
#else /* !__UEFI__ */
    printf("Boot method: BIOS\n");
#endif /* __UEFI__ */

#ifdef __UEFI__
    printf("INIT(early): initializing, total modules=%u\n",
    	   xei->sinit_end == 0 ? 1 : 2);
#else /* !__UEFI__ */
    printf("INIT(early): initializing, total modules=%u\n", mods_count);
#endif /* __UEFI__ */

    //check CPU type (Intel vs AMD)
    cpu_vendor = get_cpu_vendor_or_die(); // HALT()'s if unrecognized

#ifdef __XMHF_AMD64__
    //check whether 64-bit is supported by the CPU
    {
        uint32_t eax, edx, ebx, ecx;
        cpuid(0x80000000U, &eax, &ebx, &ecx, &edx);
        HALT_ON_ERRORCOND(eax >= 0x80000001U);
        cpuid(0x80000001U, &eax, &ebx, &ecx, &edx);
        HALT_ON_ERRORCOND((edx & (1U << 29)) && "64-bit not supported");
    }
#elif !defined(__XMHF_I386__)
    #error "Unsupported Arch"
#endif /* !defined(__XMHF_I386__) */

    //deal with MP and get CPU table
#ifdef __UEFI__
    dealwithMP((void *)(uintptr_t)xei->acpi_rsdp);
#else /* !__UEFI__ */
    dealwithMP(NULL);
#endif /* __UEFI__ */

#ifdef __UEFI__

	/*
	 * In UEFI, SL+RT is already moved to correct memory location by efi.c.
	 *
	 * We also do not need to deal with E820, because UEFI AllocatePages will
	 * hide SL+RT memory from guest for us.
	 *
	 * When __SKIP_RUNTIME_BSS__, the zero part of SL+RT is not initialized
	 * here. It is initialized in secure loader.
	 *
	 * Just set global variables, e.g. hypervisor_image_baseaddress.
	 */
	hypervisor_image_baseaddress = xei->slrt_start;
	HALT_ON_ERRORCOND((u64)hypervisor_image_baseaddress == xei->slrt_start);

	/* Set sl_rt_size */
	{
		u64 size64 = xei->slrt_end - xei->slrt_start;
		sl_rt_size = size64;
		HALT_ON_ERRORCOND((u64)sl_rt_size == size64);
	}

#else /* !__UEFI__ */

    //find highest 2MB aligned physical memory address that the hypervisor
    //binary must be moved to
    sl_rt_nonzero_size = mod_array[0].mod_end - mod_array[0].mod_start;
    sl_rt_size = sl_rt_nonzero_size;

#ifdef __SKIP_RUNTIME_BSS__
    {
        RPB *rpb = (RPB *) (mod_array[0].mod_start + 0x200000);
        sl_rt_size = PAGE_ALIGN_UP_2M((u32)rpb->XtVmmRuntimeBssEnd - __TARGET_BASE_SL);
    }
#endif /* __SKIP_RUNTIME_BSS__ */

    hypervisor_image_baseaddress = dealwithE820(mbi, PAGE_ALIGN_UP_2M((sl_rt_size)));

    //check whether multiboot modules overlap with SL+RT. mod_array[0] can
    //overlap because we will use memmove() instead of memcpy(). Currently
    //will panic if other mod_array[i] overlaps with SL+RT.
    {
        u32 i;
        _Static_assert(sizeof(hypervisor_image_baseaddress) == 4, "!");
        u32 sl_rt_start = hypervisor_image_baseaddress;
        u32 sl_rt_end;
        HALT_ON_ERRORCOND(!plus_overflow_u32(sl_rt_start, sl_rt_size));
        sl_rt_end = sl_rt_start + sl_rt_size;
        for(i=1; i < mods_count; i++) {
			HALT_ON_ERRORCOND(mod_array[i].mod_start >= sl_rt_end ||
			                  sl_rt_start >= mod_array[i].mod_end);
        }
    }

    //relocate the hypervisor binary to the above calculated address
    HALT_ON_ERRORCOND(sl_rt_nonzero_size <= sl_rt_size);
    memmove((void*)hypervisor_image_baseaddress, (void*)mod_array[0].mod_start, sl_rt_nonzero_size);

#endif /* __UEFI__ */

    HALT_ON_ERRORCOND(sl_rt_size > 0x200000); /* 2M */

#if 0 /* __NOT_RUNNING_LHV__ */
#ifndef __SKIP_BOOTLOADER_HASH__
    /* runtime */
    print_hex("    INIT(early): *UNTRUSTED* gold runtime: ",
              g_init_gold.sha_runtime, SHA_DIGEST_LENGTH);
    hashandprint("    INIT(early): *UNTRUSTED* comp runtime: ",
                 (u8*)hypervisor_image_baseaddress+0x200000, sl_rt_size-0x200000);
    /* SL low 64K */
    print_hex("    INIT(early): *UNTRUSTED* gold SL low 64K: ",
              g_init_gold.sha_slbelow64K, SHA_DIGEST_LENGTH);
    hashandprint("    INIT(early): *UNTRUSTED* comp SL low 64K: ",
                 (u8*)hypervisor_image_baseaddress, 0x10000);
    /* SL above 64K */
    print_hex("    INIT(early): *UNTRUSTED* gold SL above 64K: ",
              g_init_gold.sha_slabove64K, SHA_DIGEST_LENGTH);
    hashandprint("    INIT(early): *UNTRUSTED* comp SL above 64K): ",
                 (u8*)hypervisor_image_baseaddress+0x10000, 0x200000-0x10000);
#endif /* !__SKIP_BOOTLOADER_HASH__ */
#endif

#ifndef __UEFI__
    //print out stats
    printf("INIT(early): relocated hypervisor binary image to 0x%08lx\n", hypervisor_image_baseaddress);
    printf("INIT(early): 2M aligned size = 0x%08lx\n", PAGE_ALIGN_UP_2M((mod_array[0].mod_end - mod_array[0].mod_start)));
    printf("INIT(early): un-aligned size = 0x%08x\n", mod_array[0].mod_end - mod_array[0].mod_start);
#endif /* !__UEFI__ */

    //fill in "sl" parameter block
    {
        //"sl" parameter block is at hypervisor_image_baseaddress + 0x10000
        slpb = (SL_PARAMETER_BLOCK *)(hypervisor_image_baseaddress + 0x10000);
        HALT_ON_ERRORCOND(slpb->magic == SL_PARAMETER_BLOCK_MAGIC);
        slpb->errorHandler = 0;
        slpb->isEarlyInit = 1;    //this is an "early" init
        slpb->numE820Entries = grube820list_numentries;
        //memcpy((void *)&slpb->e820map, (void *)&grube820list, (sizeof(GRUBE820) * grube820list_numentries));
		memcpy((void *)&slpb->memmapbuffer, (void *)&grube820list, (sizeof(GRUBE820) * grube820list_numentries));
        slpb->numCPUEntries = pcpus_numentries;
        //memcpy((void *)&slpb->pcpus, (void *)&pcpus, (sizeof(PCPU) * pcpus_numentries));
        memcpy((void *)&slpb->cpuinfobuffer, (void *)&pcpus, (sizeof(PCPU) * pcpus_numentries));

#ifdef __UEFI__

        slpb->runtime_size = sl_rt_size - PAGE_SIZE_2M;

        /*
         * When UEFI, runtime_osboot* are ignored, because XMHF does not boot
         * guest OS directly.
         */
        slpb->runtime_osbootmodule_base = 0;
        slpb->runtime_osbootmodule_size = 0;
        slpb->runtime_osbootdrive = 0;

        /*
         * When UEFI, runtime_appmodule_* are ignored, because XMHF does not
         * support it yet.
         */
		slpb->runtime_appmodule_base = 0;
		slpb->runtime_appmodule_size = 0;

		slpb->uefi_acpi_rsdp = xei->acpi_rsdp;
		slpb->uefi_info = (uintptr_t)xei;

#else /* !__UEFI__ */

        slpb->runtime_size = (mod_array[0].mod_end - mod_array[0].mod_start) - PAGE_SIZE_2M;
        slpb->runtime_osbootmodule_base = 0; // mod_array[1].mod_start;
        slpb->runtime_osbootmodule_size = 0; // (mod_array[1].mod_end - mod_array[1].mod_start);
        slpb->runtime_osbootdrive = get_tboot_boot_drive();

		//check if we have an optional app module and if so populate relevant SLPB
		//fields
		{
			u32 i, start, bytes;
			slpb->runtime_appmodule_base= 0;
			slpb->runtime_appmodule_size= 0;

			//we search from module index 2 upto and including mods_count-1
			//and grab the first non-SINIT module in the list
			for(i=2; i < mods_count; i++) {
				start = mod_array[i].mod_start;
				bytes = mod_array[i].mod_end - start;
				/* Found app module */
				slpb->runtime_appmodule_base = start;
				slpb->runtime_appmodule_size = bytes;
				printf("INIT(early): found app module, base=0x%08x, size=0x%08x\n",
						slpb->runtime_appmodule_base, slpb->runtime_appmodule_size);
				break;
			}
		}

		slpb->uefi_acpi_rsdp = 0;

#endif /* __UEFI__ */

#ifdef __UEFI__
        strncpy(slpb->cmdline, xei->cmdline, sizeof(slpb->cmdline));
#else /* !__UEFI__ */
        strncpy(slpb->cmdline, (const char *)mbi->cmdline, sizeof(slpb->cmdline));
#endif /* __UEFI__ */
    }

    //switch to MP mode
    //setup Master-ID Table (MIDTABLE)
    {
        int i;
        for(i=0; i < (int)pcpus_numentries; i++){
            midtable[midtable_numentries].cpu_lapic_id = pcpus[i].lapic_id;
            midtable[midtable_numentries].vcpu_vaddr_ptr = 0;
            midtable_numentries++;
        }
    }

    //setup vcpus
    setupvcpus(cpu_vendor, midtable, midtable_numentries);

#ifdef __UEFI__
	/* Need C code help to set *init_gdt_base = init_gdt_start */
	{
		extern u64 init_gdt_base[];
		extern u64 init_gdt_start[];
		*init_gdt_base = (uintptr_t)init_gdt_start;
	}

#ifndef __SKIP_INIT_SMP__
    #error "INIT SMP in UEFI is not supported"
#endif /* __SKIP_INIT_SMP__ */

#endif /* __UEFI__ */

#ifndef __SKIP_INIT_SMP__
    //wakeup all APs
    if(midtable_numentries > 1)
        wakeupAPs();
#endif /* !__SKIP_INIT_SMP__ */

    //fall through and enter mp_cstartup via init_core_lowlevel_setup
    init_core_lowlevel_setup();

    printf("INIT(early): error(fatal), should never come here!\n");
    HALT();
}

//---isbsp----------------------------------------------------------------------
//returns 1 if the calling CPU is the BSP, else 0
u32 isbsp(void){
    u32 eax, edx;
    //read LAPIC base address from MSR
    rdmsr(MSR_APIC_BASE, &eax, &edx);
    HALT_ON_ERRORCOND( edx == 0 ); //APIC is below 4G

    if(eax & 0x100)
        return 1;
    else
        return 0;
}


//---CPUs must all have their microcode cleared for SKINIT to be successful-----
void svm_clear_microcode(VCPU *vcpu){
    u32 ucode_rev;
    u32 dummy=0;

    // Current microcode patch level available via MSR read
    rdmsr(MSR_AMD64_PATCH_LEVEL, &ucode_rev, &dummy);
    printf("CPU(0x%02x): existing microcode version 0x%08x\n", vcpu->id, ucode_rev);

    if(ucode_rev != 0) {
        wrmsr(MSR_AMD64_PATCH_CLEAR, dummy, dummy);
        printf("CPU(0x%02x): microcode CLEARED\n", vcpu->id);
    }
}


#ifndef __SKIP_INIT_SMP__
u32 cpus_active=0; //number of CPUs that are awake, should be equal to
//midtable_numentries -1 if all went well with the
//MP startup protocol
u32 lock_cpus_active=1; //spinlock to access the above
#endif /* __SKIP_INIT_SMP__ */


//------------------------------------------------------------------------------
//all cores enter here
void mp_cstartup (VCPU *vcpu){
    //sanity, we should be an Intel or AMD core
    HALT_ON_ERRORCOND(vcpu->cpu_vendor == CPU_VENDOR_INTEL ||
           vcpu->cpu_vendor == CPU_VENDOR_AMD);

	{
		extern void lhv_main(VCPU *vcpu);
		lhv_main(vcpu);
	}

	HALT_ON_ERRORCOND(0 && "Should not get here");
}
