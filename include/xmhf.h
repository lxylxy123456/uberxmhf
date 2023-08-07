#ifndef _XMHF_H_
#define _XMHF_H_

#include <_msr.h>

#ifndef __ASSEMBLY__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include <shv_types.h>

#endif	/* !__ASSEMBLY__ */

#include <_processor.h>

#ifndef __ASSEMBLY__

#include <cpu.h>

// TODO: change its name
#define HALT_ON_ERRORCOND(expr) \
	do { \
		if (!(expr)) { \
			printf("Error: HALT_ON_ERRORCOND(%s) @ %s:%d failed\n", #expr, \
				   __FILE__, __LINE__); \
			cpu_halt(); \
		} \
	} while (0)

#include <_vmx.h>
#include <_paging.h>
#include <_vmx_ctls.h>
#include <_acpi.h>
#include <hptw.h>

#include <smp.h>

/* spinlock.c */
typedef volatile uint32_t spin_lock_t;
extern void spin_lock(spin_lock_t *lock);
extern void spin_unlock(spin_lock_t *lock);

/* debug.c */
extern void *emhfc_putchar_arg;
extern spin_lock_t *emhfc_putchar_linelock_arg;
extern void emhfc_putchar(int c, void *arg);
extern void emhfc_putchar_linelock(spin_lock_t *arg);
extern void emhfc_putchar_lineunlock(spin_lock_t *arg);

#define xmhf_cpu_relax cpu_relax
#define HALT cpu_halt

typedef u64 spa_t;
typedef uintptr_t hva_t;

static inline spa_t hva2spa(void *hva)
{
	return (spa_t)(uintptr_t)hva;
}

static inline void *spa2hva(spa_t spa)
{
	return (void *)(uintptr_t)spa;
}

#define MAX_VCPU_ENTRIES 10
#define MAX_PCPU_ENTRIES (MAX_VCPU_ENTRIES)

typedef struct {
	u32 vmexit_reason;
	ulong_t guest_rip;
	u32 inst_len;
} vmexit_info_t;

#define NUM_FIXED_MTRRS 11
#define MAX_VARIABLE_MTRR_PAIRS 10

#define XMHF_GDT_SIZE 10

#define INDEX_IA32_VMX_BASIC_MSR                0x0
#define INDEX_IA32_VMX_PINBASED_CTLS_MSR        0x1
#define INDEX_IA32_VMX_PROCBASED_CTLS_MSR       0x2
#define INDEX_IA32_VMX_EXIT_CTLS_MSR            0x3
#define INDEX_IA32_VMX_ENTRY_CTLS_MSR           0x4
#define INDEX_IA32_VMX_MISC_MSR                 0x5
#define INDEX_IA32_VMX_CR0_FIXED0_MSR           0x6
#define INDEX_IA32_VMX_CR0_FIXED1_MSR           0x7
#define INDEX_IA32_VMX_CR4_FIXED0_MSR           0x8
#define INDEX_IA32_VMX_CR4_FIXED1_MSR           0x9
#define INDEX_IA32_VMX_VMCS_ENUM_MSR            0xA
#define INDEX_IA32_VMX_PROCBASED_CTLS2_MSR      0xB
#define INDEX_IA32_VMX_EPT_VPID_CAP_MSR         0xC
#define INDEX_IA32_VMX_TRUE_PINBASED_CTLS_MSR   0xD
#define INDEX_IA32_VMX_TRUE_PROCBASED_CTLS_MSR  0xE
#define INDEX_IA32_VMX_TRUE_EXIT_CTLS_MSR       0xF
#define INDEX_IA32_VMX_TRUE_ENTRY_CTLS_MSR      0x10
#define INDEX_IA32_VMX_VMFUNC_MSR               0x11
// IA32_VMX_MSRCOUNT should be 18, but Bochs does not support the last one
#define IA32_VMX_MSRCOUNT                       17

struct _guestvarmtrrmsrpair {
    u64 base;   /* IA32_MTRR_PHYSBASEi */
    u64 mask;   /* IA32_MTRR_PHYSMASKi */
};

struct _guestmtrrmsrs {
    u64 def_type;                   /* IA32_MTRR_DEF_TYPE */
    u64 fix_mtrrs[NUM_FIXED_MTRRS]; /* IA32_MTRR_FIX*, see fixed_mtrr_prop */
    u32 var_count;                  /* Number of valid var_mtrrs's */
    struct _guestvarmtrrmsrpair var_mtrrs[MAX_VARIABLE_MTRR_PAIRS];
};

typedef struct _vcpu {
	uintptr_t sp;
	u32 id;
	u32 idx;
	bool isbsp;

	u64 vmx_msrs[IA32_VMX_MSRCOUNT];
	u64 vmx_pinbased_ctls;		//IA32_VMX_PINBASED_CTLS or IA32_VMX_TRUE_...
	u64 vmx_procbased_ctls;		//IA32_VMX_PROCBASED_CTLS or IA32_VMX_TRUE_...
	u64 vmx_exit_ctls;			//IA32_VMX_EXIT_CTLS or IA32_VMX_TRUE_...
	u64 vmx_entry_ctls;			//IA32_VMX_ENTRY_CTLS or IA32_VMX_TRUE_...
	vmx_ctls_t vmx_caps;

	u8 vmx_ept_defaulttype;
	bool vmx_ept_mtrr_enable;
	bool vmx_ept_fixmtrr_enable;
	u64 vmx_ept_paddrmask;
	struct _guestmtrrmsrs vmx_guestmtrrmsrs;
	struct _vmx_vmcsfields vmcs;

	int lhv_lapic_x[2];
	int lhv_pit_x[2];
	u64 lapic_time;
	u64 pit_time;

	void *vmxon_region;
	void *my_vmcs;
	void *my_stack;
	msr_entry_t *my_vmexit_msrstore;
	msr_entry_t *my_vmexit_msrload;
	msr_entry_t *my_vmentry_msrload;
	u32 ept_exit_count;
	u8 ept_num;
	void (*volatile vmexit_handler_override)(struct _vcpu *, struct regs *,
											 vmexit_info_t *);
} VCPU;

#define SHV_STACK_SIZE (65536)

/* lhv-global.c */
extern u32 g_midtable_numentries;
extern PCPU g_cpumap[MAX_PCPU_ENTRIES];
extern MIDTAB g_midtable[MAX_VCPU_ENTRIES];
extern VCPU g_vcpus[MAX_VCPU_ENTRIES];
extern u8 g_cpu_stack[MAX_VCPU_ENTRIES][SHV_STACK_SIZE];

extern u8 g_runtime_TSS[MAX_VCPU_ENTRIES][PAGE_SIZE_4K];

#endif	/* !__ASSEMBLY__ */

#ifdef __amd64__
#define 	__CS 	0x0008 	//runtime code segment selector
#define 	__DS 	0x0018 	//runtime data segment selector
#define 	__CS32 	0x0010 	//runtime 32-bit code segment selector
#define 	__TRSEL 0x0020  //runtime TSS (task) selector
#define 	__CS_R3 0x0033  //runtime user mode code segment selector
#define 	__DS_R3 0x003b  //runtime user mode data segment selector
#elif defined(__i386__)
#define 	__CS 	0x0008 	//runtime code segment selector
#define 	__DS 	0x0010 	//runtime data segment selector
#define 	__TRSEL 0x0018  //runtime TSS (task) selector
#define 	__CS_R3 0x0023  //runtime user mode code segment selector
#define 	__DS_R3 0x002b  //runtime user mode data segment selector
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */

#define AP_BOOTSTRAP_CODE_SEG 			0x1000

#endif	/* _XMHF_H_ */
