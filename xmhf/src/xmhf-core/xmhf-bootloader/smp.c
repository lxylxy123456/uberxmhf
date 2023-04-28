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

//------------------------------------------------------------------------------
//smp.c
//this module scans for multi-core/CPUs within the system and
//returns the number of cores/CPUs as well as their LAPIC id,
//version, base and BSP indications
//author: amit vasudevan (amitvasudevan@acm.org)
#include <xmhf.h>

//forward prototypes
static int mp_checksum(unsigned char *mp, int len);
static u32 mp_scan_config(u32 base, u32 length, MPFP **mpfp);
static u32 mp_getebda(void);
ACPI_RSDP * ACPIGetRSDP(void);

static void xxd(uintptr_t start, uintptr_t end) {
	if ((start & 0xf) != 0 || (end & 0xf) != 0) {
		HALT_ON_ERRORCOND(0);
		//printf("xxd assertion failed");
		//while (1) {
		//	asm volatile ("hlt");
		//}
	}
	for (uintptr_t i = start; i < end; i += 0x10) {
		printf("LXY XXD: %08lx: ", i);
		for (uintptr_t j = 0; j < 0x10; j++) {
			if (j & 1) {
				printf("%02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			} else {
				printf(" %02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			}
		}
		printf("\n");
	}
}

#define ACPI_DESC_HEADER_SIZE 0x000000b8UL
#define ACPI_DESC_SIGNATURE_OFF 0
#define ACPI_DESC_LENGTH_OFF 4
#define ACPI_DESC_CHECKSUM_OFF 9

/*
$ sudo hexdump -C /sys/firmware/acpi/tables/DMAR
00000000  44 4d 41 52 b8 00 00 00  01 e9 49 4e 54 45 4c 20  |DMAR......INTEL |
00000010  43 50 5f 44 41 4c 45 20  01 00 00 00 49 4e 54 4c  |CP_DALE ....INTL|
00000020  01 00 00 00 23 00 00 00  00 00 00 00 00 00 00 00  |....#...........|
00000030  00 00 18 00 00 00 00 00  00 00 d9 fe 00 00 00 00  |................|
00000040  01 08 00 00 00 00 1b 00  00 00 18 00 00 00 00 00  |................|
00000050  00 10 d9 fe 00 00 00 00  01 08 00 00 00 00 02 00  |................|
00000060  00 00 10 00 01 00 00 00  00 30 d9 fe 00 00 00 00  |.........0......|
00000070  01 00 28 00 00 00 00 00  00 00 00 00 00 00 00 00  |..(.............|
00000080  ff 0f 00 00 00 00 00 00  01 08 00 00 00 00 1d 00  |................|
00000090  01 08 00 00 00 00 1a 00  01 00 20 00 00 00 00 00  |.......... .....|
000000a0  00 00 c0 bd 00 00 00 00  ff ff ff bf 00 00 00 00  |................|
000000b0  01 08 00 00 00 00 02 00                           |........|
000000b8
$ 

LXY: old
LXY XXD: bb3d0000:  444d 4152 b800 0000 01e9 494e 5445 4c20
LXY XXD: bb3d0010:  4350 5f44 414c 4520 0100 0000 494e 544c
LXY XXD: bb3d0020:  0100 0000 2300 0000 0000 0000 0000 0000
LXY XXD: bb3d0030:  0000 1800 0000 0000 0000 d9fe 0000 0000
LXY XXD: bb3d0040:  0108 0000 0000 1b00 0000 1800 0000 0000
LXY XXD: bb3d0050:  0010 d9fe 0000 0000 0108 0000 0000 0200
LXY XXD: bb3d0060:  0000 1000 0100 0000 0030 d9fe 0000 0000
LXY XXD: bb3d0070:  0100 2800 0000 0000 0000 0000 0000 0000
LXY XXD: bb3d0080:  ff0f 0000 0000 0000 0108 0000 0000 1d00
LXY XXD: bb3d0090:  0108 0000 0000 1a00 0100 2000 0000 0000
LXY XXD: bb3d00a0:  0000 c0bd 0000 0000 ffff ffbf 0000 0000
LXY XXD: bb3d00b0:  0108 0000 0000 0200 0000 0000 0000 0000
LXY: new
LXY XXD: bb3d0000:  444d 4152 b800 0000 01e9 494e 5445 4c20
LXY XXD: bb3d0010:  4350 5f44 414c 4520 0100 0000 494e 544c
LXY XXD: bb3d0020:  0100 0000 2300 0000 0000 0000 0000 0000
LXY XXD: bb3d0030:  0000 1800 0000 0000 0000 d9fe 0000 0000
LXY XXD: bb3d0040:  0108 0000 0000 1b00 0000 1800 0000 0000
LXY XXD: bb3d0050:  0030 d9fe 0000 0000 0108 0000 0000 0200
LXY XXD: bb3d0060:  0000 1000 0100 0000 0010 d9fe 0000 0000
LXY XXD: bb3d0070:  0100 2800 0000 0000 0000 0000 0000 0000
LXY XXD: bb3d0080:  ff0f 0000 0000 0000 0108 0000 0000 1d00
LXY XXD: bb3d0090:  0108 0000 0000 1a00 0100 2000 0000 0000
LXY XXD: bb3d00a0:  0000 c0bd 0000 0000 ffff ffbf 0000 0000
LXY XXD: bb3d00b0:  0108 0000 0000 0200 0000 0000 0000 0000
*/
void vmx_dmar_zap(spa_t dmaraddrphys)
{
	u8 buffer[ACPI_DESC_HEADER_SIZE];
	u8 checksum = 0;
	/* Signature */
	HALT_ON_ERRORCOND(*(u32 *)((uintptr_t)dmaraddrphys + 0x00) == 0x52414d44UL);
	/* Len */
	HALT_ON_ERRORCOND(*(u32 *)((uintptr_t)dmaraddrphys + 0x04) == ACPI_DESC_HEADER_SIZE);
	/* DRHD addresses */
	HALT_ON_ERRORCOND(*(u64 *)((uintptr_t)dmaraddrphys + 0x38) == 0xfed90000UL);
	HALT_ON_ERRORCOND(*(u64 *)((uintptr_t)dmaraddrphys + 0x50) == 0xfed91000UL);
	HALT_ON_ERRORCOND(*(u64 *)((uintptr_t)dmaraddrphys + 0x68) == 0xfed93000UL);
	/* Modify */
	*(u64 *)((uintptr_t)dmaraddrphys + 0x68) = 0xfed91000UL;
	*(u64 *)((uintptr_t)dmaraddrphys + 0x50) = 0xfed93000UL;
	/* Compute checksum */
	xmhf_baseplatform_arch_flat_copy(buffer, (u8 *)(uintptr_t)dmaraddrphys, ACPI_DESC_HEADER_SIZE);
	buffer[ACPI_DESC_CHECKSUM_OFF] = 0;
	for (size_t i = 0; i < ACPI_DESC_HEADER_SIZE; i++) {
		checksum -= buffer[i];
	}
	xmhf_baseplatform_arch_flat_writeu8(dmaraddrphys + ACPI_DESC_CHECKSUM_OFF, checksum);
}

//exposed interface to the outside world
//inputs: array of type PCPU and pointer to u32 which will
//receive the number of cores/CPUs in the system
//returns: 1 on succes, 0 on any failure
u32 smp_getinfo(PCPU *pcpus, u32 *num_pcpus){
	MPFP *mpfp;
	MPCONFTABLE *mpctable;

	ACPI_RSDP *rsdp;

#if 0
	ACPI_XSDT *xsdt;
	u32 n_xsdt_entries;
	u64 *xsdtentrylist;
#else
	ACPI_RSDT	*rsdt;
	u32 n_rsdt_entries;
	u32 *rsdtentrylist;
#endif

  ACPI_MADT *madt;
	u8 madt_found=0;
	u32 i;

	//we scan ACPI MADT and then the MP configuration table if one is
	//present, in that order!

	//if we get here it means that we did not find a MP table, so
	//we need to look at ACPI MADT. Logical cores on some machines
	//(e.g HP8540p laptop with Core i5) are reported only using ACPI MADT
	//and there is no MP structures on such systems!
	printf("Finding SMP info. via ACPI...\n");
	rsdp=(ACPI_RSDP *)ACPIGetRSDP();
	if(!rsdp){
		printf("System is not ACPI Compliant, falling through...\n");
		goto fallthrough;
	}

	printf("ACPI RSDP at 0x%08lx\n", rsdp);

#if 0
	xsdt=(ACPI_XSDT *)(u32)rsdp->xsdtaddress;
	n_xsdt_entries=(u32)((xsdt->length-sizeof(ACPI_XSDT))/8);

	printf("ACPI XSDT at 0x%08x\n", xsdt);
  printf("	len=0x%08x, headerlen=0x%08x, numentries=%u\n",
			xsdt->length, sizeof(ACPI_XSDT), n_xsdt_entries);

  xsdtentrylist=(u64 *) ( (u32)xsdt + sizeof(ACPI_XSDT) );

	for(i=0; i< n_xsdt_entries; i++){
    madt=(ACPI_MADT *)( (u32)xsdtentrylist[i]);
    if(madt->signature == ACPI_MADT_SIGNATURE){
    	madt_found=1;
    	break;
    }
	}
#else
	rsdt=(ACPI_RSDT *)(uintptr_t)rsdp->rsdtaddress;
	n_rsdt_entries=(u32)((rsdt->length-sizeof(ACPI_RSDT))/4);

	printf("ACPI RSDT at 0x%08lx\n", rsdt);
  printf("	len=0x%08x, headerlen=0x%08x, numentries=%u\n",
			rsdt->length, sizeof(ACPI_RSDT), n_rsdt_entries);

  rsdtentrylist=(u32 *) ( (uintptr_t)rsdt + sizeof(ACPI_RSDT) );

printf("LXY: old\n");
xxd(0xbb3d0000, 0xbb3d00c0);

//rsdt->length -= 4;
vmx_dmar_zap(0xbb3d0000);

printf("LXY: new\n");
xxd(0xbb3d0000, 0xbb3d00c0);

	for(i=0; i< n_rsdt_entries; i++){
    madt=(ACPI_MADT *)( (uintptr_t)rsdtentrylist[i]);
    if(madt->signature == ACPI_MADT_SIGNATURE){
    	madt_found=1;
    	break;
    }
	}

#endif


	if(!madt_found){
		printf("ACPI MADT not found, falling through...\n");
		goto fallthrough;
	}

	printf("ACPI MADT at 0x%08lx\n", madt);
	printf("	len=0x%08x, record-length=%u bytes\n", madt->length,
			madt->length - sizeof(ACPI_MADT));

	//scan through MADT APIC records to find processors
	*num_pcpus=0;
	{
		u32 madtrecordlength = madt->length - sizeof(ACPI_MADT);
		u32 madtcurrentrecordoffset=0;
		u32 i=0;
		u32 foundcores=0;

		do{
			ACPI_MADT_APIC *apicrecord = (ACPI_MADT_APIC *)((uintptr_t)madt + sizeof(ACPI_MADT) + madtcurrentrecordoffset);
 		  printf("rec type=0x%02x, length=%u bytes, flags=0x%08x, id=0x%02x\n", apicrecord->type,
			 		apicrecord->length, apicrecord->flags, apicrecord->lapicid);

			if(apicrecord->type == 0x0 && (apicrecord->flags & 0x1)){ //processor record

		    foundcores=1;
				HALT_ON_ERRORCOND( *num_pcpus < MAX_PCPU_ENTRIES);
				i = *num_pcpus;
				pcpus[i].lapic_id = apicrecord->lapicid;
		    pcpus[i].lapic_ver = 0;
		    pcpus[i].lapic_base = madt->lapicaddress;
		    if(i == 0)
					pcpus[i].isbsp = 1;	//ACPI spec says that first processor entry MUST be BSP
				else
					pcpus[i].isbsp = 0;

				*num_pcpus = *num_pcpus + 1;
			}
			madtcurrentrecordoffset += apicrecord->length;
		}while(madtcurrentrecordoffset < madtrecordlength);

		if(foundcores)
			return 1;
	}


fallthrough:
	//ok, ACPI detection failed proceed with MP table scan
	//we simply grab all the info from there as per
	//the intel MP spec.
	//look at 1K at start of conventional mem.
	//look at 1K at top of conventional mem
	//look at 1K starting at EBDA and
	//look at 64K starting at 0xF0000

	if( mp_scan_config(0x0, 0x400, &mpfp) ||
			mp_scan_config(639 * 0x400, 0x400, &mpfp) ||
			mp_scan_config(mp_getebda(), 0x400, &mpfp) ||
			mp_scan_config(0xF0000, 0x10000, &mpfp) ){

	    printf("MP table found at: 0x%08lx\n", mpfp);
  		printf("MP spec rev=0x%02x\n", mpfp->spec_rev);
  		printf("MP feature info1=0x%02x\n", mpfp->mpfeatureinfo1);
  		printf("MP feature info2=0x%02x\n", mpfp->mpfeatureinfo2);
  		printf("MP Configuration table at 0x%08x\n", mpfp->paddrpointer);

  		HALT_ON_ERRORCOND( mpfp->paddrpointer != 0 );
			mpctable = (MPCONFTABLE *)(uintptr_t)(mpfp->paddrpointer);
  		HALT_ON_ERRORCOND(mpctable->signature == MPCONFTABLE_SIGNATURE);

		  {//debug
		    int i;
		    printf("OEM ID: ");
		    for(i=0; i < 8; i++)
		      printf("%c", mpctable->oemid[i]);
		    printf("\n");
		    printf("Product ID: ");
		    for(i=0; i < 12; i++)
		      printf("%c", mpctable->productid[i]);
		    printf("\n");
		  }

		  printf("Entry count=%u\n", mpctable->entrycount);
		  printf("LAPIC base=0x%08x\n", mpctable->lapicaddr);

		  //now step through CPU entries in the MP-table to determine
		  //how many CPUs we have
		  *num_pcpus=0;

			{
		    int i;
		    uintptr_t addrofnextentry= (uintptr_t)mpctable + sizeof(MPCONFTABLE);

		    for(i=0; i < mpctable->entrycount; i++){
		      MPENTRYCPU *cpu = (MPENTRYCPU *)addrofnextentry;
		      if(cpu->entrytype != 0)
		        break;

		      if(cpu->cpuflags & 0x1){
 		        HALT_ON_ERRORCOND( *num_pcpus < MAX_PCPU_ENTRIES);
						printf("CPU (0x%08lx) #%u: lapic id=0x%02x, ver=0x%02x, cpusig=0x%08x\n",
		          cpu, i, cpu->lapicid, cpu->lapicver, cpu->cpusig);
		        pcpus[i].lapic_id = cpu->lapicid;
		        pcpus[i].lapic_ver = cpu->lapicver;
		        pcpus[i].lapic_base = mpctable->lapicaddr;
		        pcpus[i].isbsp = cpu->cpuflags & 0x2;
		        *num_pcpus = *num_pcpus + 1;
		      }

		      addrofnextentry += sizeof(MPENTRYCPU);
		    }
		  }


			return 1;
	}


	return 1;

}


static int mp_checksum(unsigned char *mp, int len){
	int sum = 0;

	while (len--)
  	sum += *mp++;

	return sum & 0xFF;
}


//returns 1 if MP table found and populates mpfp with MP table pointer
//returns 0 if no MP table and makes mpfp=NULL
static u32 mp_scan_config(u32 base, u32 length, MPFP **mpfp){
	u32 *bp = (u32 *)(uintptr_t)base;
  MPFP *mpf;

  printf("%s: Finding MP table from 0x%08lx for %u bytes\n",
                        __FUNCTION__, bp, length);

  while (length > 0) {
     mpf = (MPFP *)bp;
     if ((*bp == MPFP_SIGNATURE) &&
                    (mpf->length == 1) &&
                    !mp_checksum((unsigned char *)bp, 16) &&
                    ((mpf->spec_rev == 1)
                     || (mpf->spec_rev == 4))) {

                        printf("%s: found SMP MP-table at 0x%08lx\n",
                               __FUNCTION__, mpf);

												*mpfp = mpf;
                        return 1;
      }
     bp += 4;
     length -= 16;
  }

  *mpfp=0;
	return 0;
}


u32 mp_getebda(void){
  u16 ebdaseg;
  u32 ebdaphys;
  //get EBDA segment from 040E:0000h in BIOS data area
  ebdaseg= * ((u16 *)0x0000040E);
  //convert it to its 32-bit physical address
  ebdaphys=(u32)(ebdaseg * 16);
	return ebdaphys;
}

//------------------------------------------------------------------------------
u32 _ACPIGetRSDPComputeChecksum(u32 spaddr, u32 size){
  char *p;
  char checksum=0;
  u32 i;

  p=(char *)(uintptr_t)spaddr;

  for(i=0; i< size; i++)
    checksum+= (char)(*(p+i));

  return (u32)checksum;
}

//get the physical address of the root system description pointer (rsdp)
//return 0 if not found
ACPI_RSDP * ACPIGetRSDP(void){
  u16 ebdaseg;
  u32 ebdaphys;
  u32 i, found=0;
  ACPI_RSDP *rsdp;

  //get EBDA segment from 040E:0000h in BIOS data area
  ebdaseg= * ((u16 *)0x0000040E);
  //convert it to its 32-bit physical address
  ebdaphys=(u32)(ebdaseg * 16);
  //search first 1KB of ebda for rsdp signature (8 bytes long)
  for(i=0; i < (1024-8); i+=16){
    rsdp=(ACPI_RSDP *)(uintptr_t)(ebdaphys+i);
    if(rsdp->signature == ACPI_RSDP_SIGNATURE){
      /* Check for truncation */
      HALT_ON_ERRORCOND((uintptr_t)rsdp == (uintptr_t)(u32)(uintptr_t)rsdp);
      if(!_ACPIGetRSDPComputeChecksum((u32)(uintptr_t)rsdp, 20)){
        found=1;
        break;
      }
    }
  }

  if(found)
    return rsdp;

  //search within BIOS areas 0xE0000 to 0xFFFFF
  for(i=0xE0000; i < (0xFFFFF-8); i+=16){
    rsdp=(ACPI_RSDP *)(uintptr_t)i;
    if(rsdp->signature == ACPI_RSDP_SIGNATURE){
      HALT_ON_ERRORCOND((uintptr_t)rsdp == (uintptr_t)(u32)(uintptr_t)rsdp);
      if(!_ACPIGetRSDPComputeChecksum((u32)(uintptr_t)rsdp, 20)){
        found=1;
        break;
      }
    }
  }

  if(found)
    return rsdp;

  return (ACPI_RSDP *)NULL;
}
//------------------------------------------------------------------------------
