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
ACPI_RSDP * ACPIGetRSDP(void);
void wakeupAPs(void);
u32 smp_getinfo(PCPU *pcpus, u32 *num_pcpus, void *uefi_rsdp);

u32 _ACPIGetRSDPComputeChecksum(uintptr_t spaddr, size_t size);

static void udelay(u32 usecs){
  u8 val;
  u32 latchregval;

  //enable 8254 ch-2 counter
  val = inb(0x61);
  val &= 0x0d; //turn PC speaker off
  val |= 0x01; //turn on ch-2
  outb(0x61, val);

  //program ch-2 as one-shot
  outb(0x43, 0xB0);

  //compute appropriate latch register value depending on usecs
  latchregval = ((u64)1193182 * usecs) / 1000000;

  HALT_ON_ERRORCOND(latchregval < (1 << 16));

  //write latch register to ch-2
  val = (u8)latchregval;
  outb(0x42, val);
  val = (u8)((u32)latchregval >> (u32)8);
  outb(0x42 , val);

  #ifndef __XMHF_VERIFICATION__
  //TODO: plug in a 8254 programmable interval timer h/w model
  //wait for countdown
  while(!(inb(0x61) & 0x20)) {
    xmhf_cpu_relax();
  }
  #endif //__XMHF_VERIFICATION__

  //disable ch-2 counter
  val = inb(0x61);
  val &= 0x0c;
  outb(0x61, val);
}

void wakeupAPs(void){
	u64 apic_base;
    volatile u32 *icr;

    //read LAPIC base address from MSR
    apic_base = rdmsr64(MSR_APIC_BASE);
    HALT_ON_ERRORCOND( apic_base < ADDR_4GB ); //APIC is below 4G

    icr = (u32 *) (uintptr_t)((apic_base & ~0xFFFULL) | 0x300);

    {
        extern u32 _ap_bootstrap_start[], _ap_bootstrap_end[];
        memcpy((void *)0x10000, (void *)_ap_bootstrap_start,
               (uintptr_t)_ap_bootstrap_end - (uintptr_t)_ap_bootstrap_start);
    }

    //our test code is at 1000:0000, we need to send 10 as vector
    //send INIT
    printf("Sending INIT IPI to all APs\n");
    *icr = 0x000c4500U;
    udelay(10000);
    //wait for command completion
    while ((*icr) & 0x1000U) {
        cpu_relax();
    }
    printf("Sent INIT IPI to all APs\n");

    //send SIPI (twice as per the MP protocol)
    {
        int i;
        for(i=0; i < 2; i++){
            printf("Sending SIPI-%u\n", i);
            *icr = 0x000c4610U;
            udelay(200);
            //wait for command completion
            while ((*icr) & 0x1000U) {
                xmhf_cpu_relax();
            }
            printf("Sent SIPI-%u\n", i);
        }
    }

    printf("APs should be awake!\n");
}

//exposed interface to the outside world
//inputs: array of type PCPU and pointer to u32 which will
//receive the number of cores/CPUs in the system
//uefi_rsdp: RSDP pointer from UEFI, or NULL
//returns: 1 on succes, 0 on any failure
u32 smp_getinfo(PCPU *pcpus, u32 *num_pcpus, void *uefi_rsdp){
  ACPI_RSDP *rsdp;

#if 0
  ACPI_XSDT *xsdt;
  u32 n_xsdt_entries;
  u64 *xsdtentrylist;
#else
  ACPI_RSDT *rsdt;
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
  if (uefi_rsdp == NULL) {
    rsdp=(ACPI_RSDP *)ACPIGetRSDP();
  } else {
    rsdp = (ACPI_RSDP *)uefi_rsdp;
    HALT_ON_ERRORCOND(_ACPIGetRSDPComputeChecksum((uintptr_t)rsdp, 20) == 0);
  }
  if(!rsdp){
    printf("System is not ACPI Compliant, falling through...\n");
    goto fallthrough;
  }

  printf("ACPI RSDP at 0x%08lx\n", rsdp);

#if 0
  xsdt=(ACPI_XSDT *)(u32)rsdp->xsdtaddress;
  n_xsdt_entries=(u32)((xsdt->length-sizeof(ACPI_XSDT))/8);

  printf("ACPI XSDT at 0x%08x\n", xsdt);
  printf("  len=0x%08x, headerlen=0x%08x, numentries=%u\n",
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
  printf("  len=0x%08x, headerlen=0x%08x, numentries=%u\n",
      rsdt->length, sizeof(ACPI_RSDT), n_rsdt_entries);

  rsdtentrylist=(u32 *) ( (uintptr_t)rsdt + sizeof(ACPI_RSDT) );

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
  printf("  len=0x%08x, record-length=%u bytes\n", madt->length,
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
          pcpus[i].isbsp = 1; //ACPI spec says that first processor entry MUST be BSP
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
  HALT_ON_ERRORCOND(0 && "ACPI detection failed");


  return 1;
}

//------------------------------------------------------------------------------
u32 _ACPIGetRSDPComputeChecksum(uintptr_t spaddr, size_t size){
  char *p;
  char checksum=0;
  size_t i;

  p=(char *)spaddr;

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
      if(!_ACPIGetRSDPComputeChecksum((uintptr_t)rsdp, 20)){
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
      if(!_ACPIGetRSDPComputeChecksum((uintptr_t)rsdp, 20)){
        found=1;
        break;
      }
    }
  }

  if(found)
    return rsdp;

  return (ACPI_RSDP *)NULL;
}

void smp_init(void)
{
	/* Get list of CPUs information. */
	HALT_ON_ERRORCOND(smp_getinfo(g_cpumap, &g_midtable_numentries, NULL));
	for (u32 i = 0; i < g_midtable_numentries; i++) {
		g_midtable[i].cpu_lapic_id = g_cpumap[i].lapic_id;
		g_midtable[i].vcpu_vaddr_ptr = (uintptr_t)&g_vcpus[i];
		g_vcpus[i].sp = (uintptr_t)(g_cpu_stack[i + 1]);
		g_vcpus[i].id = g_cpumap[i].lapic_id;
		g_vcpus[i].idx = i;
		g_vcpus[i].isbsp = g_cpumap[i].isbsp;
	}

	/* Wake up APs. */
	wakeupAPs();

	/* Switch stack and call kernel_main_smp(). */
	{
		extern void init_core_lowlevel_setup(void);
		init_core_lowlevel_setup();
	}

	HALT_ON_ERRORCOND(0 && "Should not return from init_core_lowlevel_setup()");
}
