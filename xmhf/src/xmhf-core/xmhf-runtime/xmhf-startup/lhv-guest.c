#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define INTERRUPT_PERIOD 20

/*
 * An interrupt handler or VMEXIT handler will see exit_source. If it sees
 * other than EXIT_IGNORE or EXIT_MEASURE, it will error. If it sees
 * EXIT_MEASURE, it will put one of EXIT_NMI, EXIT_TIMER, or EXIT_VMEXIT to
 * exit_source.
 */
enum exit_source {
	EXIT_IGNORE,	/* Ignore the interrupt */
	EXIT_MEASURE,	/* Measure the interrupt */
	EXIT_NMI_G,		/* Interrupt comes from guest NMI interrupt handler */
	EXIT_TIMER_G,	/* Interrupt comes from guest timer interrupt handler */
	EXIT_NMI_H,		/* Interrupt comes from host NMI interrupt handler */
	EXIT_TIMER_H,	/* Interrupt comes from host timer interrupt handler */
	EXIT_VMEXIT,	/* Interrupt comes from NMI VMEXIT handler */
};

const char *exit_source_str[] = {
	"EXIT_IGNORE  (0)",
	"EXIT_MEASURE (1)",
	"EXIT_NMI_G   (2)",
	"EXIT_TIMER_G (3)",
	"EXIT_NMI_H   (4)",
	"EXIT_TIMER_H (5)",
	"EXIT_VMEXIT  (6)",
	"OVERFLOW",
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

#define TEST_ASSERT(_p) \
    do { \
        if ( !(_p) ) { \
            master_fail = __LINE__; \
            l2_ready = 0; \
            printf("\nTEST_ASSERT '%s' failed, line %d, file %s\n", #_p , __LINE__, __FILE__); \
            HALT(); \
        } \
    } while (0)

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
		printf("      Interrupt recorded:       %s\n", exit_source_str[source]);
		exit_source = source;
		exit_rip = rip;
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
				printf("      Inject NMI\n");
				*icr_low = 0x00004400U;
				break;
			case INTERRUPT_PERIOD:
				printf("      Inject interrupt\n");
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
		handle_interrupt_cpu1(EXIT_VMEXIT, guest_rip + inst_len);
		vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
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

static void set_state(bool nmi_exiting, bool virtual_nmis, bool blocking_by_nmi)
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
		u32 guest_int = vmcs_vmread(NULL, VMCS_guest_interruptibility);
		guest_int &= ~0x8U;
		if (blocking_by_nmi) {
			guest_int |= 0x8U;
		}
		vmcs_vmwrite(NULL, VMCS_guest_interruptibility, guest_int);
	}
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

/* Execute HLT and expect interrupt to hit on the instruction */
void hlt_wait(u32 source)
{
	uintptr_t rip;
	printf("    hlt_wait() begin, source =  %s\n", exit_source_str[source]);
	prepare_measure();
	exit_source = EXIT_MEASURE;
	l2_ready = 1;
loop:
	asm volatile ("pushf; sti; hlt; 1: leal 1b, %0; popf" : "=g"(rip));
	if ("qemu workaround" && exit_source == EXIT_MEASURE) {
		printf("      Strange wakeup from HLT\n");
		goto loop;
	}
	l2_ready = 0;
	assert_measure(source, rip);
	printf("    hlt_wait() end\n");
}

/* Execute IRET and expect interrupt to hit on the instruction */
void iret_wait(u32 source)
{
	uintptr_t rip;
	printf("    iret_wait() begin, source = %s\n", exit_source_str[source]);
	prepare_measure();
	exit_source = EXIT_MEASURE;
	rip = xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip();
	assert_measure(source, rip);
	printf("    iret_wait() end\n");
}

void lhv_vmcall_main(void)
{
	printf("  Enter host, exp=%d, state=%d\n", experiment_no, state_no);
	switch (experiment_no) {
	case 1:
		set_state(0, 0, 0);
		break;
	case 2:
		switch (state_no) {
		case 0:
			set_state(0, 0, 1);
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
		break;
	case 3:
		switch (state_no) {
		case 0:
			hlt_wait(EXIT_NMI_H);
			/* Do not unblock NMI */
			hlt_wait(EXIT_TIMER_H);
			hlt_wait(EXIT_TIMER_H);
			set_state(0, 0, 0);
			/* Expecting EXIT_NMI_G after VMENTRY */
			prepare_measure();
			break;
		default:
			TEST_ASSERT(0 && "unexpected state");
			break;
		}
		break;
	case 4:
		switch (state_no) {
		case 0:
			set_state(0, 0, 1);
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
		break;
	default:
		TEST_ASSERT(0 && "unexpected experiment");
		break;
	}
	printf("  Leave host\n");
}

/*
 * Experiment 1: NMI Exiting = 0, virtual NMIs = 0
 * NMI will cause NMI interrupt handler in guest.
 */
void experiment_1(void)
{
	printf("Experiment: %d\n", (experiment_no = 1));
	asm volatile ("vmcall");
	/* Make sure NMI hits HLT */
	hlt_wait(EXIT_NMI_G);
	xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip();
	/* Reset state by letting Timer hit HLT */
	hlt_wait(EXIT_TIMER_G);
}

/*
 * Experiment 2: NMI Exiting = 0, virtual NMIs = 0
 * L2 (guest) blocks NMI, L1 (LHV) does not. L2 receives NMI, then VMEXIT to
 * L1. Result: L1 does not receive NMI.
 * This test does not work on QEMU.
 */
void experiment_2(void)
{
	printf("Experiment: %d\n", (experiment_no = 2));
	state_no = 0;
	asm volatile ("vmcall");
	/* An NMI should be blocked, then a timer hits HLT */
	hlt_wait(EXIT_TIMER_G);
	/* Now VMENTRY to L1 */
	exit_source = EXIT_MEASURE;
	state_no = 1;
	asm volatile ("vmcall");
	/* Unblock NMI. There are 2 NMIs, but only 1 delivered due to blocking. */
	iret_wait(EXIT_NMI_G);
	iret_wait(EXIT_MEASURE);
}

/*
 * Experiment 3: NMI Exiting = 0, virtual NMIs = 0
 * L1 (guest) blocks NMI, L2 (LHV) does not. L1 receives NMI, then VMENTRY to
 * L2. Result: L2 receives NMI after VMENTRY.
 * This test does not work on Bochs.
 * This test does not work on QEMU ("experiment_3(); experiment_3();" fails).
 */
void experiment_3(void)
{
	uintptr_t rip;
	printf("Experiment: %d\n", (experiment_no = 3));
	state_no = 0;
	asm volatile ("vmcall; 1: leal 1b, %0" : "=g"(rip));
	assert_measure(EXIT_NMI_G, rip);
	xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip();
}

/*
 * Experiment 4: NMI Exiting = 0, virtual NMIs = 0
 * L1 (guest) does not block NMI, sets VMCS to make L2 (LHV) block NMI. Then
 * L2 VMEXIT to L1, and L1 expects an interrupt. Result: L1 does not get NMI.
 * This test does not work on QEMU.
 */
void experiment_4(void)
{
	printf("Experiment: %d\n", (experiment_no = 4));
	state_no = 0;
	asm volatile ("vmcall");
	state_no = 1;
	asm volatile ("vmcall");
	xmhf_smpguest_arch_x86vmx_unblock_nmi_with_rip();
}

static struct {
	void (*f)(void);
	bool support_qemu;
	bool support_bochs;
} experiments[] = {
	{NULL, true, true},
	{experiment_1, true, true},
	{experiment_2, false, true},
	{experiment_3, false, false},
	{experiment_4, false, true},
};

static u32 nexperiments = sizeof(experiments) / sizeof(experiments[0]);

static bool in_qemu = false;
static bool in_bochs = false;

void run_experiment(u32 i)
{
	if (in_qemu && !experiments[i].support_qemu) {
		printf("Skipping experiments[%d] due to QEMU\n", i);
		return;
	}
	if (in_bochs && !experiments[i].support_bochs) {
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
#ifndef __DEBUG_VGA__
		u32 eax, ebx, ecx, edx;
		cpuid(0x1, &eax, &ebx, &ecx, &edx);
		if (ecx & 0x80000000U) {
			in_qemu = true;
		} else {
			in_bochs = true;
		}
#endif
	}
	asm volatile ("sti");
	if (0 && "sequential") {
		for (u32 i = 0; i < nexperiments; i++) {
			run_experiment(i);
		}
	}
	if (0 && "random") {
		for (u32 i = 0; i < 100000; i++) {
			run_experiment(rand() % nexperiments);
		}
	}
	{
		experiment_4();
	}
	{
		TEST_ASSERT(!master_fail);
		printf("TEST PASS\nTEST PASS\nTEST PASS\n");
		l2_ready = 0;
		HALT();
	}
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
