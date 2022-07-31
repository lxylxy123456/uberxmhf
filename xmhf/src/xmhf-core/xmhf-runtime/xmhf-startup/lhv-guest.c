#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define INTERRUPT_PERIOD 10

/*
 * An interrupt handler or VMEXIT handler will see exit_source. If it sees
 * other than EXIT_IGNORE or EXIT_MEASURE, it will error. If it sees
 * EXIT_MEASURE, it will put one of EXIT_NMI, EXIT_TIMER, or EXIT_VMEXIT to
 * exit_source.
 */
enum exit_source {
	EXIT_IGNORE,	/* Ignore the interrupt */
	EXIT_MEASURE,	/* Measure the interrupt */
	EXIT_MEAS_2,	/* Measure two interrupts */
	EXIT_NMI_G,		/* Interrupt comes from guest NMI interrupt handler */
	EXIT_TIMER_G,	/* Interrupt comes from guest timer interrupt handler */
	EXIT_NMI_H,		/* Interrupt comes from host NMI interrupt handler */
	EXIT_TIMER_H,	/* Interrupt comes from host timer interrupt handler */
	EXIT_VMEXIT,	/* Interrupt comes from NMI VMEXIT handler */
	EXIT_NMIWIND,	/* Interrupt comes from NMI Windowing handler */
};

const char *exit_source_str[] = {
	"EXIT_IGNORE  (0)",
	"EXIT_MEASURE (1)",
	"EXIT_MEAS_2  (2)",
	"EXIT_NMI_G   (3)",
	"EXIT_TIMER_G (4)",
	"EXIT_NMI_H   (5)",
	"EXIT_TIMER_H (6)",
	"EXIT_VMEXIT  (7)",
	"EXIT_NMIWIND (8)",
	"OVERFLOW",
	"OVERFLOW",
	"OVERFLOW",
};

static volatile u32 l2_ready = 0;
static volatile u32 master_fail = 0;
static volatile u32 experiment_no = 0;
static volatile u32 state_no = 0;
static volatile enum exit_source exit_source = EXIT_MEASURE;
static volatile uintptr_t exit_rip = 0;
static volatile enum exit_source exit_source_old = EXIT_MEASURE;
static volatile uintptr_t exit_rip_old = 0;
static volatile bool quiet = false;

#define TEST_ASSERT(_p) \
    do { \
        if ( !(_p) ) { \
            master_fail = __LINE__; \
            l2_ready = 0; \
            printf("\nTEST_ASSERT '%s' failed, line %d, file %s\n", #_p , __LINE__, __FILE__); \
            HALT(); \
        } \
    } while (0)

extern void XtRtmIdtStub2(void);
extern void XtRtmIdtStub21(void);

/* https://zh.wikipedia.org/wiki/%E4%BC%AA%E9%9A%8F%E6%9C%BA%E6%80%A7 */
u32 rand(void)
{
	static u32 seed = 12;
	seed = seed * 1103515245 + 12345;
	return (seed / 65536) % 32768;
}

void handle_interrupt_cpu1(u32 source, uintptr_t rip)
{
	HALT_ON_ERRORCOND(!master_fail);
	switch (exit_source) {
	case EXIT_IGNORE:
		printf("      Interrupt ignored:        %s\n", exit_source_str[source]);
		TEST_ASSERT(0 && "Should not ignore interrupt");
		break;
	case EXIT_MEASURE:
		if (!quiet) {
			printf("      Interrupt recorded:       %s\n",
				   exit_source_str[source]);
		}
		exit_source = source;
		exit_rip = rip;
		break;
	case EXIT_MEAS_2:
		printf("      Interrupt 1 recorded:     %s\n", exit_source_str[source]);
		exit_source = EXIT_MEASURE;
		exit_source_old = source;
		exit_rip_old = rip;
		break;
	default:
		printf("\nsource:      %s", exit_source_str[source]);
		printf("\nexit_source: %s", exit_source_str[exit_source]);
		TEST_ASSERT(0 && "Fail: unexpected exit_source");
		break;
	}
}

void handle_nmi_interrupt(VCPU *vcpu, int vector, int guest, uintptr_t rip)
{
	HALT_ON_ERRORCOND(!vcpu->isbsp);
	HALT_ON_ERRORCOND(vector == 0x2);
	if (guest) {
		handle_interrupt_cpu1(EXIT_NMI_G, rip);
	} else {
		handle_interrupt_cpu1(EXIT_NMI_H, rip);
	}
	// printf("NMI %d\n", guest);
	(void) guest;
}

void handle_timer_interrupt(VCPU *vcpu, int vector, int guest, uintptr_t rip)
{
	if (vcpu->isbsp) {
		HALT_ON_ERRORCOND(vector == 0x20);
		HALT_ON_ERRORCOND(guest == 0);
		if (l2_ready) {
			static u32 count = 0;
			volatile u32 *icr_high = (volatile u32 *)(0xfee00310);
			volatile u32 *icr_low = (volatile u32 *)(0xfee00300);
			*icr_high = 0x01000000U;
			switch ((count++) % (INTERRUPT_PERIOD * 2)) {
			case 0:
				if (!quiet) {
					printf("      Inject NMI\n");
				}
				*icr_low = 0x00004400U;
				break;
			case INTERRUPT_PERIOD:
				if (!quiet) {
					printf("      Inject interrupt\n");
				}
				*icr_low = 0x00004022U;
				break;
			default:
				break;
			}
		}
		outb(INT_ACK_CURRENT, INT_CTL_PORT);
	} else {
		HALT_ON_ERRORCOND(vector == 0x22);
		if (guest) {
			handle_interrupt_cpu1(EXIT_TIMER_G, rip);
		} else {
			handle_interrupt_cpu1(EXIT_TIMER_H, rip);
		}
		// printf("TIMER %d\n", guest);
		write_lapic(LAPIC_EOI, 0);
	}
}

void vmexit_handler(VCPU *vcpu, struct regs *r)
{
	ulong_t vmexit_reason = vmcs_vmread(vcpu, VMCS_info_vmexit_reason);
	ulong_t guest_rip = vmcs_vmread(vcpu, VMCS_guest_RIP);
	ulong_t inst_len = vmcs_vmread(vcpu, VMCS_info_vmexit_instruction_length);
	HALT_ON_ERRORCOND(vcpu == _svm_and_vmx_getvcpu());
	switch (vmexit_reason) {
	case VMX_VMEXIT_CPUID:
		{
			u32 old_eax = r->eax;
			asm volatile ("cpuid\r\n"
				  :"=a"(r->eax), "=b"(r->ebx), "=c"(r->ecx), "=d"(r->edx)
				  :"a"(r->eax), "c" (r->ecx));
			if (old_eax == 0x1) {
				/* Clear VMX capability */
				r->ecx &= ~(1U << 5);
			}
			vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
			break;
		}
	case VMX_VMEXIT_VMCALL:
		// printf("VMCALL\n");
		{
			void lhv_vmcall_main(void);
			lhv_vmcall_main();
		}
		vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
		break;
	case VMX_VMEXIT_EXCEPTION:
		TEST_ASSERT((vmcs_vmread(vcpu, VMCS_info_vmexit_interrupt_information) &
					 INTR_INFO_VECTOR_MASK) == 0x2);
		handle_interrupt_cpu1(EXIT_VMEXIT, guest_rip);
		if (0) {
			u32 activ_state = vmcs_vmread(vcpu, VMCS_guest_activity_state);
			printf("Activity state: %d\n", activ_state);
			printf("Inst len:       %d\n", inst_len);
			printf("Guest RIP:      0x%08x\n", guest_rip);
		}
		switch (vmcs_vmread(vcpu, VMCS_guest_activity_state)) {
		case 0:
			/* CPU is active */
			break;
		case 1:
			/* Wake CPU from HLT state */
			vmcs_vmwrite(vcpu, VMCS_guest_activity_state, 0);
			break;
		default:
			TEST_ASSERT(0 && "Unknown activity state");
			break;
		}
		break;
	case VMX_VMEXIT_NMI_WINDOW:
		handle_interrupt_cpu1(EXIT_NMIWIND, guest_rip);
		{
			u32 ctl_cpu_base = vmcs_vmread(NULL, VMCS_control_VMX_cpu_based);
			TEST_ASSERT(ctl_cpu_base &
						(1U << VMX_PROCBASED_NMI_WINDOW_EXITING));
			ctl_cpu_base &= ~(1U << VMX_PROCBASED_NMI_WINDOW_EXITING);
			vmcs_vmwrite(NULL, VMCS_control_VMX_cpu_based, ctl_cpu_base);
		}
		break;
	default:
		{
			printf("CPU(0x%02x): unknown vmexit %d\n", vcpu->id, vmexit_reason);
			printf("CPU(0x%02x): rip = 0x%x\n", vcpu->id, guest_rip);
			vmcs_dump(vcpu, 0);
			HALT_ON_ERRORCOND(0);
			break;
		}
	}
	vmresume_asm(r);
}

uintptr_t xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip(void) {
	uintptr_t rip;
#ifdef __AMD64__
    asm volatile (
        "movq    %%rsp, %%rsi   \r\n"
        "xorq    %%rax, %%rax   \r\n"
        "movw    %%ss, %%ax     \r\n"
        "pushq   %%rax          \r\n"
        "pushq   %%rsi          \r\n"
        "pushfq                 \r\n"
        "xorq    %%rax, %%rax   \r\n"
        "movw    %%cs, %%ax     \r\n"
        "pushq   %%rax          \r\n"
        "pushq   $1f            \r\n"
        "iretq                  \r\n"
        "1: leaq 1b, %0         \r\n"
        : "=g"(rip)
        : // no input
        : "%rax", "%rsi", "cc", "memory");
#elif defined(__I386__)
    asm volatile (
        "pushfl                 \r\n"
        "xorl    %%eax, %%eax   \r\n"
        "movw    %%cs, %%ax     \r\n"
        "pushl   %%eax          \r\n"
        "pushl   $1f            \r\n"
        "iretl                  \r\n"
        "1: leal 1b, %0         \r\n"
        : "=g"(rip)
        : // no input
        : "%eax", "cc", "memory");
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	return rip;
}

static bool get_blocking_by_nmi(void)
{
	u32 guest_int = vmcs_vmread(NULL, VMCS_guest_interruptibility);
	return (guest_int & 0x8U);
}

static void assert_state(bool nmi_exiting, bool virtual_nmis,
						 bool blocking_by_nmi, bool nmi_windowing)
{
	{
		u32 ctl_pin_base = vmcs_vmread(NULL, VMCS_control_VMX_pin_based);
		if (nmi_exiting) {
			TEST_ASSERT((ctl_pin_base & 0x8U) != 0);
		} else {
			TEST_ASSERT((ctl_pin_base & 0x8U) == 0);
		}
		if (virtual_nmis) {
			TEST_ASSERT((ctl_pin_base & 0x20U) != 0);
		} else {
			TEST_ASSERT((ctl_pin_base & 0x20U) == 0);
		}
	}
	{
		u32 ctl_cpu_base = vmcs_vmread(NULL, VMCS_control_VMX_cpu_based);
		if (nmi_windowing) {
			TEST_ASSERT((ctl_cpu_base &
						 (1U << VMX_PROCBASED_NMI_WINDOW_EXITING)) != 0);
		} else {
			TEST_ASSERT((ctl_cpu_base &
						 (1U << VMX_PROCBASED_NMI_WINDOW_EXITING)) == 0);
		}
	}
	{
		u32 guest_int = vmcs_vmread(NULL, VMCS_guest_interruptibility);
		if (blocking_by_nmi) {
			TEST_ASSERT((guest_int & 0x8U) != 0);
		} else {
			TEST_ASSERT((guest_int & 0x8U) == 0);
		}
	}
}

static void set_state(bool nmi_exiting, bool virtual_nmis, bool blocking_by_nmi,
					  bool nmi_windowing)
{
	{
		u32 ctl_pin_base = vmcs_vmread(NULL, VMCS_control_VMX_pin_based);
		ctl_pin_base &= ~0x28U;
		if (nmi_exiting) {
			ctl_pin_base |= 0x8U;
		}
		if (virtual_nmis) {
			ctl_pin_base |= 0x20U;
		}
		vmcs_vmwrite(NULL, VMCS_control_VMX_pin_based, ctl_pin_base);
	}
	{
		u32 ctl_cpu_base = vmcs_vmread(NULL, VMCS_control_VMX_cpu_based);
		ctl_cpu_base &= ~(1U << VMX_PROCBASED_NMI_WINDOW_EXITING);
		if (nmi_windowing) {
			ctl_cpu_base |= (1U << VMX_PROCBASED_NMI_WINDOW_EXITING);
		}
		vmcs_vmwrite(NULL, VMCS_control_VMX_cpu_based, ctl_cpu_base);
	}
	{
		u32 guest_int = vmcs_vmread(NULL, VMCS_guest_interruptibility);
		guest_int &= ~0x8U;
		if (blocking_by_nmi) {
			guest_int |= 0x8U;
		}
		vmcs_vmwrite(NULL, VMCS_guest_interruptibility, guest_int);
	}
}

static void set_inject_nmi(void)
{
	u32 val = vmcs_vmread(NULL, VMCS_control_VM_entry_interruption_information);
	TEST_ASSERT((val & 0x80000000) == 0);
	vmcs_vmwrite(NULL, VMCS_control_VM_entry_interruption_information,
				 0x80000202);
}

/* Prepare for assert_measure() */
static void prepare_measure(void)
{
	TEST_ASSERT(exit_source == EXIT_MEASURE);
	TEST_ASSERT(exit_rip == 0);
}

/* Assert an interrupt source happens at rip */
static void assert_measure(u32 source, uintptr_t rip)
{
	if (exit_source != source) {
		printf("\nsource:      %s", exit_source_str[source]);
		printf("\nexit_source: %s", exit_source_str[exit_source]);
		TEST_ASSERT(0 && (exit_source == source));
	}
	if (exit_source != EXIT_MEASURE) {
		TEST_ASSERT(rip == exit_rip);
	}
	exit_source = EXIT_MEASURE;
	exit_rip = 0;
}

/* Prepare for assert_measure_2() */
static void prepare_measure_2(void)
{
	TEST_ASSERT(exit_source == EXIT_MEASURE);
	TEST_ASSERT(exit_rip == 0);
	exit_source = EXIT_MEAS_2;
	exit_rip = 0;
	exit_source_old = EXIT_MEASURE;
	exit_rip_old = 0;
}

/* Assert an interrupt source happens at rip */
static void assert_measure_2(u32 source1, uintptr_t rip1, u32 source2,
							 uintptr_t rip2)
{
	if (exit_source_old != source1) {
		printf("\nsource1:         %s", exit_source_str[source1]);
		printf("\nexit_source_old: %s", exit_source_str[exit_source_old]);
		TEST_ASSERT(0 && (exit_source_old == source1));
	}
	if (exit_source_old != EXIT_MEASURE) {
		TEST_ASSERT(rip1 == exit_rip_old);
	}
	if (exit_source != source2) {
		printf("\nsource2:     %s", exit_source_str[source2]);
		printf("\nexit_source: %s", exit_source_str[exit_source]);
		TEST_ASSERT(0 && (exit_source == source2));
	}
	if (exit_source != EXIT_MEASURE) {
		TEST_ASSERT(rip2 == exit_rip);
	}
	exit_source = EXIT_MEASURE;
	exit_rip = 0;
	exit_source_old = EXIT_MEASURE;
	exit_rip_old = 0;
}

/* Execute HLT and expect interrupt to hit on the instruction */
void hlt_wait(u32 source)
{
	uintptr_t rip;
	if (!quiet) {
		printf("    hlt_wait() begin, source =  %s\n", exit_source_str[source]);
	}
	prepare_measure();
	exit_source = EXIT_MEASURE;
	l2_ready = 1;
loop:
	asm volatile ("pushf; sti; hlt; 1: leal 1b, %0; popf" : "=g"(rip));
	if ("qemu workaround" && exit_source == EXIT_MEASURE) {
		if (!quiet) {
			printf("      Strange wakeup from HLT\n");
		}
		goto loop;
	}
	l2_ready = 0;
	assert_measure(source, rip);
	if (!quiet) {
		printf("    hlt_wait() end\n");
	}
}

/* Execute IRET and expect interrupt to hit on the instruction */
void iret_wait(u32 source)
{
	uintptr_t rip;
	if (!quiet) {
		printf("    iret_wait() begin, source = %s\n", exit_source_str[source]);
	}
	prepare_measure();
	exit_source = EXIT_MEASURE;
	rip = xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip();
	assert_measure(source, rip);
	if (!quiet) {
		printf("    iret_wait() end\n");
	}
}

/*
 * Experiment 1: NMI Exiting = 0, virtual NMIs = 0
 * NMI will cause NMI interrupt handler in guest.
 */
static void experiment_1(void)
{
	printf("Experiment: %d\n", (experiment_no = 1));
	state_no = 0;
	asm volatile ("vmcall");
	/* Make sure NMI hits HLT */
	hlt_wait(EXIT_NMI_G);
	iret_wait(EXIT_MEASURE);
	/* Reset state by letting Timer hit HLT */
	hlt_wait(EXIT_TIMER_G);
}

static void experiment_1_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 2: NMI Exiting = 0, virtual NMIs = 0
 * L2 (guest) blocks NMI, L1 (LHV) does not. L2 receives NMI, then VMEXIT to
 * L1. Result: L1 does not receive NMI.
 * This test does not work on QEMU.
 */
static void experiment_2(void)
{
	printf("Experiment: %d\n", (experiment_no = 2));
	state_no = 0;
	asm volatile ("vmcall");
	/* An NMI should be blocked, then a timer hits HLT */
	hlt_wait(EXIT_TIMER_G);
	/* Now VMEXIT to L1 */
	exit_source = EXIT_MEASURE;
	state_no = 1;
	prepare_measure();
	asm volatile ("vmcall");
	/* Unblock NMI. There are 2 NMIs, but only 1 delivered due to blocking. */
	iret_wait(EXIT_NMI_G);
	iret_wait(EXIT_MEASURE);
}

static void experiment_2_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(0, 0, 1, 0);
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		assert_measure(EXIT_MEASURE, 0);
		hlt_wait(EXIT_TIMER_H);
		/* Looks like NMI is also blocked on host. */
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 3: NMI Exiting = 0, virtual NMIs = 0
 * L1 (guest) blocks NMI, L2 (LHV) does not. L1 receives NMI, then VMENTRY to
 * L2. Result: L2 receives NMI after VMENTRY.
 * This test does not work on Bochs.
 * This test does not work on QEMU ("experiment_3(); experiment_3();" fails).
 */
static void experiment_3(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 3));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	iret_wait(EXIT_MEASURE);
}

static void experiment_3_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		/* Do not unblock NMI */
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		/* Expecting EXIT_NMI_G after VMENTRY */
		prepare_measure();
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 4: NMI Exiting = 0, virtual NMIs = 0
 * L1 (guest) does not block NMI, sets VMCS to make L2 (LHV) block NMI. Then
 * L2 VMEXIT to L1, and L1 expects an interrupt. Result: L1 does not get NMI.
 * This test does not work on QEMU.
 */
static void experiment_4(void)
{
	printf("Experiment: %d\n", (experiment_no = 4));
	state_no = 0;
	asm volatile ("vmcall");
	state_no = 1;
	asm volatile ("vmcall");
	iret_wait(EXIT_MEASURE);
}

static void experiment_4_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(0, 0, 1, 0);
		break;
	case 1:
		hlt_wait(EXIT_TIMER_H);
		iret_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 5: NMI Exiting = 1, virtual NMIs = 0
 * L2 (guest) does not block NMI. When NMI hits L2, VMEXIT should happen.
 * Result: VMEXIT happens.
 * This test may not work on Bochs ("experiment_5(); experiment_1();" fails).
 */
static void experiment_5(void)
{
	printf("Experiment: %d\n", (experiment_no = 5));
	state_no = 0;
	asm volatile ("vmcall");
	hlt_wait(EXIT_VMEXIT);
	state_no = 1;
	asm volatile ("vmcall");
	hlt_wait(EXIT_TIMER_G);
}

static void experiment_5_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 0, 0, 0);
		break;
	case 1:
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 6: NMI Exiting = 1, virtual NMIs = 0
 * L2 (guest) blocks NMI. When NMI hits L2. Result: VMEXIT does not happen.
 * Then, execute IRET in guest: Result: VMEXIT still does not happen
 * Then, execute IRET in host: Result: VMEXIT happen in host when IRET
 * This test does not work on QEMU.
 */
static void experiment_6(void)
{
	printf("Experiment: %d\n", (experiment_no = 6));
	state_no = 0;
	asm volatile ("vmcall");
	hlt_wait(EXIT_TIMER_G);
	iret_wait(EXIT_MEASURE);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_6_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 0, 1, 0);
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		iret_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		set_state(1, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 7: NMI Exiting = 1, virtual NMIs = 0
 * L1 (host) blocks NMI, NMI hits L1. L2 (guest) does not block NMI in VMCS.
 * L1 VMENTRY to L2 (guest), Result: L2 receives NMI VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_7(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 7));
	state_no = 0;
	prepare_measure();
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_VMEXIT, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_7_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 0, 0, 0);
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 8: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) does not block NMI. When NMI hits L2, VMEXIT should happen.
 * Result: VMEXIT happens.
 * This is similar to experiment 5, but enable virtual NMI.
 */
static void experiment_8(void)
{
	printf("Experiment: %d\n", (experiment_no = 8));
	state_no = 0;
	asm volatile ("vmcall");
	hlt_wait(EXIT_VMEXIT);
	state_no = 1;
	asm volatile ("vmcall");
	hlt_wait(EXIT_TIMER_G);
}

static void experiment_8_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 0, 0);
		break;
	case 1:
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 9: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) blocks virtual NMI. When NMI hits L2. Result: VMEXIT happens.
 */
static void experiment_9(void)
{
	printf("Experiment: %d\n", (experiment_no = 9));
	state_no = 0;
	asm volatile ("vmcall");
	hlt_wait(EXIT_VMEXIT);
	hlt_wait(EXIT_TIMER_G);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_9_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 1, 0);
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 10: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) blocks NMI, NMI hits L1. L2 (guest) does not block NMI in VMCS.
 * L1 VMENTRY to L2 (guest), Result: L2 receives NMI VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_10(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 10));
	state_no = 0;
	prepare_measure();
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_VMEXIT, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_10_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 0, 0);
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 11: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) blocks NMI, NMI hits L1. L2 (guest) blocks virtual NMI in VMCS.
 * L1 VMENTRY to L2 (guest), Result: L2 receives NMI VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_11(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 11));
	state_no = 0;
	prepare_measure();
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_VMEXIT, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_11_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 1, 0);
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 12: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) blocks virtual NMI in VMCS, then executes IRET.
 * Result: L2 no longer blocks virtual NMI.
 */
static void experiment_12(void)
{
	printf("Experiment: %d\n", (experiment_no = 12));
	state_no = 0;
	asm volatile ("vmcall");
	iret_wait(EXIT_MEASURE);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_12_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 1, 0);
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 13: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) blocks NMI. L2 (guest) does not block NMI. L1 VMENTRY to L2, then
 * L2 VMEXIT to L1. Then L1 expects an interrupt. Result: L1 gets NMI.
 * This test does not work on Bochs.
 * This test does not work on QEMU.
 */
static void experiment_13(void)
{
	printf("Experiment: %d\n", (experiment_no = 13));
	state_no = 0;
	asm volatile ("vmcall");
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_13_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 0, 0);
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 14: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) blocks NMI. L2 (guest) blocks virtual NMI. L1 VMENTRY to L2, then
 * L2 VMEXIT to L1. Then L1 expects an interrupt. Result: L1 gets NMI.
 * This test does not work on Bochs.
 * This test does not work on QEMU.
 */
static void experiment_14(void)
{
	printf("Experiment: %d\n", (experiment_no = 14));
	state_no = 0;
	asm volatile ("vmcall");
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_14_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 1, 0);
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 15: NMI Exiting = 0, virtual NMIs = 0
 * L1 (host) blocks NMI. L2 (guest) does not block NMI. L1 VMENTRY to L2, and
 * inject a keyboard interrupt at the same time. Result: L2 executes the first
 * instruction of the keyboard interrupt before NMI.
 * This test does not work on Bochs.
 */
static void experiment_15(void)
{
	printf("Experiment: %d\n", (experiment_no = 15));
	state_no = 0;
	asm volatile ("vmcall");
	assert_measure(EXIT_NMI_G, (uintptr_t) XtRtmIdtStub21);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_15_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		vmcs_vmwrite(NULL, VMCS_control_VM_entry_interruption_information,
					 0x80000021);
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 16: NMI Exiting = 1, virtual NMIs = 0
 * L1 (host) blocks NMI. L2 (guest) does not block NMI. L1 VMENTRY to L2, and
 * inject a keyboard interrupt at the same time. Result: L2 executes the first
 * instruction of the keyboard interrupt before NMI VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_16(void)
{
	printf("Experiment: %d\n", (experiment_no = 16));
	state_no = 0;
	asm volatile ("vmcall");
	assert_measure(EXIT_VMEXIT, (uintptr_t) XtRtmIdtStub21);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_16_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 0, 0, 0);
		vmcs_vmwrite(NULL, VMCS_control_VM_entry_interruption_information,
					 0x80000021);
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 17: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) blocks NMI. L2 (guest) does not block NMI. L1 VMENTRY to L2, and
 * inject a keyboard interrupt at the same time. Result: L2 executes the first
 * instruction of the keyboard interrupt before NMI VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_17(void)
{
	printf("Experiment: %d\n", (experiment_no = 17));
	state_no = 0;
	asm volatile ("vmcall");
	assert_measure(EXIT_VMEXIT, (uintptr_t) XtRtmIdtStub21);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_17_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 1, 0);
		vmcs_vmwrite(NULL, VMCS_control_VM_entry_interruption_information,
					 0x80000021);
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 18: NMI Exiting = 0, virtual NMIs = 0
 * L1 (host) does not block NMI. L2 (guest) does not block NMI. L1 injects an
 * NMI during VMENTRY to L2. Result: L2 does not blocks NMI after injection.
 * This test does not work on Bochs.
 * This test does not work on QEMU.
 */
static void experiment_18(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 18));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_18_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		set_inject_nmi();
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 19: NMI Exiting = 1, virtual NMIs = 0
 * L1 (host) does not block NMI. L2 (guest) does not block NMI. L1 injects an
 * NMI during VMENTRY to L2. Result: L2 does not block NMI after injection.
 * This test does not work on Bochs.
 * This test does not work on QEMU.
 */
static void experiment_19(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 19));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_19_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 0, 0, 0);
		set_inject_nmi();
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(!get_blocking_by_nmi());
		/* Make sure that host also does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 20: NMI Exiting = 1, virtual NMIs = 1
 * L1 (host) does not block NMI. L2 (guest) does not block virtual NMI. L1
 * injects an NMI during VMENTRY to L2. Result: L2 blocks virtual NMI after
 * injection.
 */
static void experiment_20(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 20));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_20_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 0, 0);
		set_inject_nmi();
		prepare_measure();
		break;
	case 1:
		TEST_ASSERT(get_blocking_by_nmi());
		/* Make sure that host does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 21: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) does not block virtual NMI. L1 sets NMI windowing bit in VMCS.
 * Result: after VMENTRY to L2, an NMI windowing exit happens.
 */
static void experiment_21(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 21));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMIWIND, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_21_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 0, 1);
		prepare_measure();
		break;
	case 1:
		assert_state(1, 1, 0, 0);
		/* Make sure that host does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 22: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) blocks virtual NMI. L1 sets NMI windowing bit in VMCS.
 * Result: after VMENTRY to L2, no NMI windowing exit happens.
 */
static void experiment_22(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 22));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_MEASURE, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_22_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 1, 1);
		prepare_measure();
		break;
	case 1:
		assert_state(1, 1, 1, 1);
		/* Make sure that host does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 23: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) blocks virtual NMI. L1 sets NMI windowing bit in VMCS. After
 * VMENTRY to L2, L2 executes IRET. Result: NMI windowing happens right after
 * IRET in L2.
 */
static void experiment_23(void)
{
	printf("Experiment: %d\n", (experiment_no = 23));
	state_no = 0;
	asm volatile ("vmcall");
	assert_measure(EXIT_MEASURE, 0);
	iret_wait(EXIT_NMIWIND);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_23_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 1, 1);
		prepare_measure();
		break;
	case 1:
		assert_state(1, 1, 0, 0);
		/* Make sure that host does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 24: NMI Exiting = 1, virtual NMIs = 1
 * L1 (LHV) blocks NMI. L2 (guest) does not block virtual NMI. L1 receives an
 * NMI, then sets NMI windowing bit in VMCS and VMENTRY. See which event
 * happens first. Result: NMI windowing happens first, then NMI hits the host.
 * This test does not work on Bochs.
 * This test does not work on QEMU.
 */
static void experiment_24(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 24));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	/*
	 * Note: VMEXIT is recorded before NMI injection because after NMI
	 * injection is completed, VMEXIT happens. After VMEXIT completes, the
	 * first instruction of NMI injection is executed.
	 */
	assert_measure_2(EXIT_NMI_H, (uintptr_t) vmexit_asm, EXIT_NMIWIND, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_24_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 0, 1);
		prepare_measure_2();
		break;
	case 1:
		assert_state(1, 1, 0, 0);
		/* Make sure that host does not block NMI */
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 25: NMI Exiting = 1, virtual NMIs = 1
 * L2 (guest) does not block virtual NMI. L1 sets NMI windowing bit in VMCS and
 * injects an NMI during VMENTRY. See which event happens first.
 * Result: NMI is injected first, then virtual NMIs are blocked. Guest needs
 * to execute IRET to cause NMI windowing VMEXIT.
 */
static void experiment_25(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 25));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	iret_wait(EXIT_NMIWIND);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_25_vmcall(void)
{
	switch (state_no) {
	case 0:
		set_state(1, 1, 0, 1);
		set_inject_nmi();
		prepare_measure();
		break;
	case 1:
		assert_state(1, 1, 0, 0);
		/* Make sure that host does not block NMI */
		hlt_wait(EXIT_NMI_H);
		iret_wait(EXIT_MEASURE);
		hlt_wait(EXIT_TIMER_H);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

/*
 * Experiment 26: NMI Exiting = 1, virtual NMIs = 1
 * L1 (LHV) blocks NMI. L2 (guest) does not block virtual NMI. L1 receives NMI,
 * and injects an NMI during VMENTRY. See which event happens first.
 * Result: the NMI injection happens first, then VMEXIT.
 * This test does not work on Bochs.
 */
static void experiment_26(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 26));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	/*
	 * Note: VMEXIT is recorded before NMI injection because after NMI
	 * injection is completed, VMEXIT happens. After VMEXIT completes, the
	 * first instruction of NMI injection is executed.
	 */
	assert_measure_2(EXIT_VMEXIT, (uintptr_t) XtRtmIdtStub2, EXIT_NMI_G, rip);
	state_no = 1;
	asm volatile ("vmcall");
}

static void experiment_26_vmcall(void)
{
	switch (state_no) {
	case 0:
		hlt_wait(EXIT_NMI_H);
		hlt_wait(EXIT_TIMER_H);
		hlt_wait(EXIT_TIMER_H);
		set_state(1, 1, 0, 0);
		set_inject_nmi();
		prepare_measure_2();
		break;
	case 1:
		assert_state(1, 1, 1, 0);
		/* Make sure that host does not block NMI */
		iret_wait(EXIT_MEASURE);
		set_state(0, 0, 0, 0);
		break;
	default:
		TEST_ASSERT(0 && "unexpected state");
		break;
	}
}

static struct {
	void (*f)(void);
	void (*vmcall)(void);
	bool supported;
	bool support_xmhf;
	bool support_qemu;
	bool support_bochs;
} experiments[] = { /*                    a  x  q  b */
	{NULL,          NULL,                 1, 0, 0, 0},
	{experiment_1,  experiment_1_vmcall,  1, 1, 1, 1},
	{experiment_2,  experiment_2_vmcall,  1, 1, 0, 1},
	{experiment_3,  experiment_3_vmcall,  1, 1, 0, 0},
	{experiment_4,  experiment_4_vmcall,  1, 1, 0, 1},
	{experiment_5,  experiment_5_vmcall,  1, 1, 1, 1},
	{experiment_6,  experiment_6_vmcall,  1, 1, 0, 1},
	{experiment_7,  experiment_7_vmcall,  1, 1, 1, 0},
	{experiment_8,  experiment_8_vmcall,  1, 1, 1, 1},
	{experiment_9,  experiment_9_vmcall,  1, 1, 1, 1},
	{experiment_10, experiment_10_vmcall, 1, 1, 1, 0},
	{experiment_11, experiment_11_vmcall, 1, 1, 1, 0},
	{experiment_12, experiment_12_vmcall, 1, 1, 1, 1},
	{experiment_13, experiment_13_vmcall, 1, 1, 0, 0},
	{experiment_14, experiment_14_vmcall, 1, 1, 0, 0},
	{experiment_15, experiment_15_vmcall, 1, 1, 1, 0},
	{experiment_16, experiment_16_vmcall, 1, 1, 1, 0},
	{experiment_17, experiment_17_vmcall, 1, 1, 1, 0},
	{experiment_18, experiment_18_vmcall, 1, 0, 0, 0},
	{experiment_19, experiment_19_vmcall, 1, 1, 0, 0},
	{experiment_20, experiment_20_vmcall, 1, 1, 1, 1},
	{experiment_21, experiment_21_vmcall, 1, 1, 1, 1},
	{experiment_22, experiment_22_vmcall, 1, 1, 1, 1},
	{experiment_23, experiment_23_vmcall, 1, 1, 1, 1},
	{experiment_24, experiment_24_vmcall, 1, 1, 0, 0},
	{experiment_25, experiment_25_vmcall, 1, 1, 1, 1},
	{experiment_26, experiment_26_vmcall, 1, 0, 1, 0},
	/*                                    a  x  q  b */
};

static u32 nexperiments = sizeof(experiments) / sizeof(experiments[0]);

static bool in_qemu = false;
static bool in_bochs = false;
static bool in_xmhf = false;

void run_experiment(u32 i)
{
	if (!experiments[i].supported) {
		printf("Skipping experiments[%d] due to global settings\n", i);
		return;
	}
	if (in_xmhf && !experiments[i].support_xmhf) {
		printf("Skipping experiments[%d] due to XMHF\n", i);
		return;
	}
	if (in_qemu && !in_xmhf && !experiments[i].support_qemu) {
		printf("Skipping experiments[%d] due to QEMU\n", i);
		return;
	}
	if (in_bochs && !in_xmhf && !experiments[i].support_bochs) {
		printf("Skipping experiments[%d] due to Bochs\n", i);
		return;
	}
	if (experiments[i].f) {
		experiments[i].f();
	}
	TEST_ASSERT(!master_fail);
}

void lhv_guest_main(ulong_t cpu_id)
{
	TEST_ASSERT(cpu_id == 1);
	{
		u32 eax, ebx, ecx, edx;
		printf("Detecting environment\n");
		/*
		 * I am not sure of a good way to detect Bochs. For now just use the
		 * current CPU version information I see.
		 */
		cpuid(0x00000001U, &eax, &ebx, &ecx, &edx);
		if (eax == 0x000206a7) {
			in_bochs = true;
			printf("    Bochs detected\n");
		}
		/*
		 * Detect QEMU / KVM using
		 * https://01.org/linuxgraphics/gfx-docs/drm/virt/kvm/cpuid.html
		 */
		cpuid(0x40000000U, &eax, &ebx, &ecx, &edx);
		if (ebx == 0x4b4d564b && ecx == 0x564b4d56 && edx == 0x0000004d) {
			in_qemu = true;
			printf("    QEMU / KVM detected\n");
		}
		/* Detect XMHF */
		cpuid(0x46484d58U, &eax, &ebx, &ecx, &edx);
		if (eax == 0x46484d58U) {
			in_xmhf = true;
			printf("    XMHF detected\n");
		}
		printf("End detecting environment\n");
		/* Wait for some time to make the results visible */
		if ("Sleep") {
			quiet = true;
			for (u32 i = 0; i < 30; i += INTERRUPT_PERIOD) {
				hlt_wait(EXIT_NMI_G);
				iret_wait(EXIT_MEASURE);
				hlt_wait(EXIT_TIMER_G);
			}
			quiet = false;
		}
	}
	asm volatile ("sti");
	if (1 && "hardcode") {
		experiment_25();
	}
	if (1 && "sequential") {
		for (u32 i = 0; i < nexperiments; i++) {
			run_experiment(i);
		}
	}
	if (1 && "random") {
		for (u32 i = 0; i < 100000; i++) {
			run_experiment(rand() % nexperiments);
		}
	}
	{
		TEST_ASSERT(!master_fail);
		printf("TEST PASS\nTEST PASS\nTEST PASS\n");
		l2_ready = 0;
		HALT();
	}
}

void lhv_vmcall_main(void)
{
	printf("  Enter host, exp=%d, state=%d\n", experiment_no, state_no);
	TEST_ASSERT(experiment_no < nexperiments);
	TEST_ASSERT(experiments[experiment_no].vmcall);
	experiments[experiment_no].vmcall();
	printf("  Leave host\n");
}

#if 0
void lhv_guest_main_old(ulong_t cpu_id)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	console_vc_t vc;
	HALT_ON_ERRORCOND(cpu_id == vcpu->idx);
	console_get_vc(&vc, vcpu->idx, 1);
	console_clear(&vc);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		asm volatile ("sti");
	}
	while (1) {
		if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
			asm volatile ("hlt");
		}
		asm volatile ("vmcall");
		if (__LHV_OPT__ & LHV_USE_EPT) {
			u32 a = 0xdeadbeef;
			u32 *p = (u32 *)0x12340000;
			asm volatile("movl (%1), %%eax" :
						 "+a" (a) :
						 "b" (p) :
						 "cc", "memory");
			if (0) {
				printf("CPU(0x%02x): EPT result: 0x%08x 0x%02x\n", vcpu->id, a,
					   vcpu->ept_num);
			}
			if (vcpu->ept_num == 0) {
				HALT_ON_ERRORCOND(a == 0xfee1c0de);
			} else {
				HALT_ON_ERRORCOND((u8) a == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 8) == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 16) == vcpu->ept_num);
				HALT_ON_ERRORCOND((u8) (a >> 24) == vcpu->ept_num);
			}
		}
		if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
#ifdef __AMD64__
			extern void lhv_disable_enable_paging(char *);
			if ("quiet") {
				lhv_disable_enable_paging("");
			} else {
				lhv_disable_enable_paging("LHV guest can disable paging\n");
			}
#elif defined(__I386__)
			ulong_t cr0 = read_cr0();
			asm volatile ("cli");
			write_cr0(cr0 & 0x7fffffffUL);
			if (0) {
				printf("CPU(0x%02x): LHV guest can disable paging\n", vcpu->id);
			}
			write_cr0(cr0);
			asm volatile ("sti");
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
		}
	}
}
#endif

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	uintptr_t rip;
	(void) r;
#ifdef __AMD64__
	rip = *(uintptr_t *)r->rsp;
#elif defined(__I386__)
	rip = *(uintptr_t *)r->esp;
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	switch (vector) {
	case 0x2:
		{
			extern void handle_nmi_interrupt(VCPU *vcpu, int vector, int guest,
											 uintptr_t rip);
			handle_nmi_interrupt(_svm_and_vmx_getvcpu(), vector, 1, rip);
		}
		break;
	case 0x20:
		{
			extern void handle_timer_interrupt(VCPU *vcpu, int vector,
											   int guest, uintptr_t rip);
			handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1, rip);
		}
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x22:
		{
			extern void handle_timer_interrupt(VCPU *vcpu, int vector,
											   int guest, uintptr_t rip);
			handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1, rip);
		}
		break;
	default:
		printf("Guest: interrupt / exception vector %ld\n", vector);
		HALT_ON_ERRORCOND(0 && "Guest: unknown interrupt / exception!\n");
		break;
	}
}
