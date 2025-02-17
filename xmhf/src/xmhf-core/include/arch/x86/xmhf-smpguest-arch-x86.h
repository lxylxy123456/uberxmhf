/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * Copyright (c) 2023 Eric Li
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

// EMHF smpguest component
// x86 arch. specific declarations
// author: amit vasudevan (amitvasudevan@acm.org)

#ifndef __EMHF_SMPGUEST_ARCH_X86_H__
#define __EMHF_SMPGUEST_ARCH_X86_H__


/* Macros that control how NMIs for the guest are handled */
#define SMPG_VMX_NMI_INJECT     0   /* Inject NMI to guest */
#ifdef __NESTED_VIRTUALIZATION__
#define SMPG_VMX_NMI_NESTED     1   /* Let nested virtualization decide */
#endif /* !__NESTED_VIRTUALIZATION__ */


#ifndef __ASSEMBLY__


//----------------------------------------------------------------------
//ARCH. BACKENDS
//----------------------------------------------------------------------

//initialize SMP guest logic
void xmhf_smpguest_arch_initialize(VCPU *vcpu);

//quiesce interface to switch all guest cores into hypervisor mode
void xmhf_smpguest_arch_quiesce(VCPU *vcpu);

//endquiesce interface to resume all guest cores after a quiesce
void xmhf_smpguest_arch_endquiesce(VCPU *vcpu);

//walk guest page tables; returns pointer to corresponding guest physical address
//note: returns 0xFFFFFFFF if there is no mapping
u8 * xmhf_smpguest_arch_walk_pagetables(VCPU *vcpu, u32 vaddr);

// Inject NMI to the guest when the guest is ready to receive it (i.e. when the
// guest is not running NMI handler)
// The NMI window VMEXIT is used to make sure the guest is able to receive NMIs
//
// This function should be called in intercept handlers (a.k.a. VMEXIT
// handlers). Otherwise, the caller needs to make sure that this function is
// called after xmhf_smpguest_arch_x86vmx_mhv_nmi_disable().
void xmhf_smpguest_arch_inject_nmi(VCPU *vcpu);

// Block NMIs using software
// This function must be called in intercept handlers (a.k.a. VMEXIT handlers),
// because it edits VMCS through vcpu->vmcs and expects the intercept handler
// to write the update to the hardware VMCS later.
void xmhf_smpguest_arch_nmi_block(VCPU *vcpu);

// Unblock NMIs using software
// This function must be called in intercept handlers (a.k.a. VMEXIT handlers),
// because it edits VMCS through vcpu->vmcs and expects the intercept handler
// to write the update to the hardware VMCS later.
void xmhf_smpguest_arch_nmi_unblock(VCPU *vcpu);


//----------------------------------------------------------------------
//x86 ARCH. INTERFACES
//----------------------------------------------------------------------

//perform required setup after a guest awakens a new CPU
void xmhf_smpguest_arch_x86_postCPUwakeup(VCPU *vcpu);

//handle LAPIC access #DB (single-step) exception event
void xmhf_smpguest_arch_x86_eventhandler_dbexception(VCPU *vcpu,
	struct regs *r);

//handle LAPIC access #NPF (nested page fault) event
void xmhf_smpguest_arch_x86_eventhandler_hwpgtblviolation(VCPU *vcpu, u32 gpa, u32 errorcode);

//quiescing handler for #NMI (non-maskable interrupt) exception event
void xmhf_smpguest_arch_x86_eventhandler_nmiexception(VCPU *vcpu, struct regs *r, u32 from_guest);


//----------------------------------------------------------------------
//x86vmx SUBARCH. INTERFACES
//----------------------------------------------------------------------

void xmhf_smpguest_arch_x86vmx_initialize(VCPU *vcpu, u32 unmaplapic);
void xmhf_smpguest_arch_x86vmx_link_lapic(VCPU *vcpu);
void xmhf_smpguest_arch_x86vmx_eventhandler_dbexception(VCPU *vcpu, struct regs *r);
int xmhf_smpguest_arch_x86vmx_eventhandler_x2apic_icrwrite(VCPU *vcpu, u64 value);
u32 xmhf_smpguest_arch_x86vmx_nmi_check_quiesce(VCPU *vcpu);
void xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception(VCPU *vcpu, struct regs *r, u32 from_guest);
u32 xmhf_smpguest_arch_x86vmx_eventhandler_hwpgtblviolation(VCPU *vcpu, u32 paddr, u32 errorcode);
void xmhf_smpguest_arch_x86vmx_unblock_nmi(void);
void xmhf_smpguest_arch_x86vmx_quiesce(VCPU *vcpu);
void xmhf_smpguest_arch_x86vmx_endquiesce(VCPU *vcpu);

// Check whether xmhf_smpguest_arch_x86vmx_mhv_nmi_disable() is in effect
bool xmhf_smpguest_arch_x86vmx_mhv_nmi_disabled(VCPU *vcpu);
// Handle NMI for the guest received in XMHF's NMI exception handler
void xmhf_smpguest_arch_x86vmx_mhv_nmi_handle(VCPU *vcpu);
// Temporarily block NMI during XMHF's intercept handler
void xmhf_smpguest_arch_x86vmx_mhv_nmi_disable(VCPU *vcpu);
// Unblock NMI in XMHF's intercept handler
void xmhf_smpguest_arch_x86vmx_mhv_nmi_enable(VCPU *vcpu);

// Update NMI window exiting bit in VMCS control_VMX_cpu_based
void xmhf_smpguest_arch_x86vmx_update_nmi_window_exiting(VCPU *vcpu,
														 u32 *procctl);

// Inject NMI to the guest when the guest is ready to receive it (i.e. when the
// guest is not running NMI handler)
// The NMI window VMEXIT is used to make sure the guest is able to receive NMIs
//
// This function should be called in intercept handlers (a.k.a. VMEXIT
// handlers). Otherwise, the caller needs to make sure that this function is
// called after xmhf_smpguest_arch_x86vmx_mhv_nmi_disable().
void xmhf_smpguest_arch_x86vmx_inject_nmi(VCPU *vcpu);

// Block NMIs using software
// This function must be called in intercept handlers (a.k.a. VMEXIT handlers),
// because it edits VMCS through vcpu->vmcs and expects the intercept handler
// to write the update to the hardware VMCS later.
void xmhf_smpguest_arch_x86vmx_nmi_block(VCPU *vcpu);

// Unblock NMIs using software
// This function must be called in intercept handlers (a.k.a. VMEXIT handlers),
// because it edits VMCS through vcpu->vmcs and expects the intercept handler
// to write the update to the hardware VMCS later.
void xmhf_smpguest_arch_x86vmx_nmi_unblock(VCPU *vcpu);

//perform required setup after a guest awakens a new CPU
void xmhf_smpguest_arch_x86vmx_postCPUwakeup(VCPU *vcpu);
//walk guest page tables; returns pointer to corresponding guest physical address
//note: returns 0xFFFFFFFF if there is no mapping
u8 * xmhf_smpguest_arch_x86vmx_walk_pagetables(VCPU *vcpu, u32 vaddr);

//the BSP LAPIC base address
extern u32 g_vmx_lapic_base __attribute__(( section(".data") ));

//4k buffer which is the virtual LAPIC page that guest reads and writes from/to
//during INIT-SIPI-SIPI emulation
extern u8 g_vmx_virtual_LAPIC_base[] __attribute__((aligned(PAGE_SIZE_4K)));

//the quiesce counter, all CPUs except for the one requesting the
//quiesce will increment this when they get their quiesce signal
extern u32 volatile g_vmx_quiesce_counter __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 volatile g_vmx_lock_quiesce_counter __attribute__(( section(".data") ));

//resume counter to rally all CPUs after resumption from quiesce
extern u32 volatile g_vmx_quiesce_resume_counter __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 volatile g_vmx_lock_quiesce_resume_counter __attribute__(( section(".data") ));

//the "quiesce" variable, if 1, then we have a quiesce in process
extern u32 volatile g_vmx_quiesce __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 volatile g_vmx_lock_quiesce __attribute__(( section(".data") ));

//resume signal, becomes 1 to signal resume after quiescing
extern u32 volatile g_vmx_quiesce_resume_signal __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 volatile g_vmx_lock_quiesce_resume_signal __attribute__(( section(".data") ));

//Flush all EPT TLB on all cores
//smpguest x86vmx
extern u32 volatile g_vmx_flush_all_tlb_signal __attribute__(( section(".data") ));


//----------------------------------------------------------------------
//x86svm SUBARCH. INTERFACES
//----------------------------------------------------------------------

void xmhf_smpguest_arch_x86svm_initialize(VCPU *vcpu);
void xmhf_smpguest_arch_x86svm_eventhandler_dbexception(VCPU *vcpu,
	struct regs *r);
void xmhf_smpguest_arch_x86svm_eventhandler_nmiexception(VCPU *vcpu, struct regs *r);
u32 xmhf_smpguest_arch_x86svm_eventhandler_hwpgtblviolation(VCPU *vcpu, u32 paddr, u32 errorcode);
void xmhf_smpguest_arch_x86svm_quiesce(VCPU *vcpu);
void xmhf_smpguest_arch_x86svm_endquiesce(VCPU *vcpu);


//perform required setup after a guest awakens a new CPU
void xmhf_smpguest_arch_x86svm_postCPUwakeup(VCPU *vcpu);
//walk guest page tables; returns pointer to corresponding guest physical address
//note: returns 0xFFFFFFFF if there is no mapping
u8 * xmhf_smpguest_arch_x86svm_walk_pagetables(VCPU *vcpu, u32 vaddr);

//the BSP LAPIC base address
extern u32 g_svm_lapic_base __attribute__(( section(".data") ));

//the quiesce counter, all CPUs except for the one requesting the
//quiesce will increment this when they get their quiesce signal
extern u32 g_svm_quiesce_counter __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 g_svm_lock_quiesce_counter __attribute__(( section(".data") ));

//resume counter to rally all CPUs after resumption from quiesce
extern u32 g_svm_quiesce_resume_counter __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 g_svm_lock_quiesce_resume_counter __attribute__(( section(".data") ));

//the "quiesce" variable, if 1, then we have a quiesce in process
extern u32 g_svm_quiesce __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 g_svm_lock_quiesce __attribute__(( section(".data") ));

//resume signal, becomes 1 to signal resume after quiescing
extern u32 g_svm_quiesce_resume_signal __attribute__(( section(".data") ));

//SMP lock to access the above variable
extern u32 g_svm_lock_quiesce_resume_signal __attribute__(( section(".data") ));

//4k buffer which is the virtual LAPIC page that guest reads and writes from/to
//during INIT-SIPI-SIPI emulation
extern u8 g_svm_virtual_LAPIC_base[] __attribute__((aligned(PAGE_SIZE_4K)));

//SVM SIPI page buffers used for guest INIT-SIPI-SIPI emulation
extern u8 g_svm_sipi_page_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));




#endif	//__ASSEMBLY__

#endif // __EMHF_SMPGUEST_ARCH_X86_H__
