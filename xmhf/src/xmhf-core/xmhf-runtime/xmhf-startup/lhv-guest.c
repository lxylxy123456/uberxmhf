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
#define EXIT_IGNORE		0	/* Ignore the interrupt */
#define EXIT_MEASURE	1	/* Measure the interrupt */
#define EXIT_NMI		2	/* Interrupt comes from NMI interrupt handler */
#define EXIT_TIMER		3	/* Interrupt comes from timer interrupt handler */
#define EXIT_VMEXIT		4	/* Interrupt comes from NMI VMEXIT handler */

static volatile u32 l2_ready = 0;
static volatile u32 master_fail = 0;
static volatile u32 experiment_no = 0;
static volatile u32 state_no = 0;
static volatile u32 exit_source = EXIT_IGNORE;
static volatile uintptr_t exit_rip = 0;

#define TEST_ASSERT(_p) \
    do { \
        if ( !(_p) ) { \
            master_fail = __LINE__; \
            printf("\nTEST_ASSERT '%s' failed, line %d, file %s\n", #_p , __LINE__, __FILE__); \
            HALT(); \
        } \
    } while (0)

void handle_interrupt_cpu1(u32 source, uintptr_t rip)
{
	HALT_ON_ERRORCOND(!master_fail);
	switch (exit_source) {
	case EXIT_IGNORE:
		printf("Interrupt %d ignored\n", source);
		break;
	case EXIT_MEASURE:
		printf("Interrupt %d recorded\n", source);
		exit_source = source;
		exit_rip = rip;
		break;
	default:
		TEST_ASSERT(0 && "Fail: unexpected exit_source");
		break;
	}
}

void handle_nmi_interrupt(VCPU *vcpu, int vector, int guest, uintptr_t rip)
{
	HALT_ON_ERRORCOND(!vcpu->isbsp);
	HALT_ON_ERRORCOND(vector == 0x2);
	handle_interrupt_cpu1(EXIT_NMI, rip);
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
				printf("Inject NMI\n");
				*icr_low = 0x00004400U;
				break;
			case INTERRUPT_PERIOD:
				printf("Inject interrupt\n");
				*icr_low = 0x00004022U;
				break;
			default:
				break;
			}
		}
		outb(INT_ACK_CURRENT, INT_CTL_PORT);
	} else {
		HALT_ON_ERRORCOND(vector == 0x22);
		handle_interrupt_cpu1(EXIT_TIMER, rip);
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

void set_state(bool nmi_exiting, bool virtual_nmis, bool blocking_by_nmi)
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

void hlt_wait(u32 source)
{
	uintptr_t rip;
	TEST_ASSERT(exit_rip == 0);
	TEST_ASSERT(exit_source == EXIT_IGNORE);
	exit_source = EXIT_MEASURE;
	l2_ready = 1;
	asm volatile ("hlt; 1: leal 1b, %0;" : "=g"(rip));
	l2_ready = 0;
	TEST_ASSERT(rip == exit_rip);
	TEST_ASSERT(exit_source == source);
	exit_source = EXIT_IGNORE;
	exit_rip = 0;
}

void lhv_vmcall_main(void)
{
	switch (experiment_no) {
	case 1:
		set_state(0, 0, 0);
		break;
	case 2:
		set_state(0, 0, 1);
		break;
	default:
		TEST_ASSERT(0 && "unexpected experiment");
		break;
	}
}

void lhv_guest_main(ulong_t cpu_id)
{
	TEST_ASSERT(cpu_id == 1);
	asm volatile ("sti");
	if ("experiment 1") {
		/*
		 * Experiment 1: NMI Exiting = 0, virtual NMIs = 0
		 * NMI will cause NMI interrupt handler in guest.
		 */
		experiment_no = 1;
		asm volatile ("vmcall");
		/* Make sure NMI hits HLT */
		hlt_wait(EXIT_NMI);
		/* Reset state by letting Timer hit HLT */
		hlt_wait(EXIT_TIMER);
		exit_source = EXIT_IGNORE;
	}
	if ("experiment 2") {
		/*
		 * Experiment 2: NMI Exiting = 0, virtual NMIs = 0
		 * L2 (guest) blocks NMI, L1 (LHV) does not. L2 receives NMI, then
		 * VMENTRY to L1.
		 */
		experiment_no = 2;
		asm volatile ("vmcall");
		/* An NMI should be blocked, then a timer hits HLT */
		// hlt_wait(EXIT_TIMER);
		// TODO
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
