#include <xmhf.h>
#include <lhv.h>
#include <lhv-pic.h>

#define INTERRUPT_PERIOD 20

static volatile u32 l2_ready = 0;

void handle_nmi_interrupt(VCPU *vcpu, int vector, int guest)
{
	HALT_ON_ERRORCOND(!vcpu->isbsp);
	HALT_ON_ERRORCOND(vector == 0x2);
	printf("NMI %d\n", guest);
}

void handle_timer_interrupt(VCPU *vcpu, int vector, int guest)
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
		printf("TIMER %d\n", guest);
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
		printf("VMCALL\n");
		HALT_ON_ERRORCOND(0 && "TODO frontier");
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

void lhv_guest_main(ulong_t cpu_id)
{
	(void) cpu_id;
	l2_ready = 1;
	while (1) {
		asm volatile ("sti");
		asm volatile ("hlt");
	}
	asm volatile ("vmcall");
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
	(void) r;
	switch (vector) {
	case 0x2:
    	handle_nmi_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
    	break;
	case 0x20:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x22:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	default:
		printf("Guest: interrupt / exception vector %ld\n", vector);
		HALT_ON_ERRORCOND(0 && "Guest: unknown interrupt / exception!\n");
		break;
	}
}
