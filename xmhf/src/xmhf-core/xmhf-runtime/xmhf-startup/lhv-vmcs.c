#include <xmhf.h>
#include <lhv.h>

// TODO: optimize by caching

void vmcs_vmwrite(VCPU *vcpu, ulong_t encoding, ulong_t value)
{
	(void) vcpu;
	// printf("\nCPU(0x%02x): vmwrite(0x%04lx, 0x%08lx)", vcpu->id, encoding, value);
	HALT_ON_ERRORCOND(__vmx_vmwrite(encoding, value));
}

ulong_t vmcs_vmread(VCPU *vcpu, ulong_t encoding)
{
	unsigned long value;
	(void) vcpu;
	HALT_ON_ERRORCOND(__vmx_vmread(encoding, &value));
	// printf("\nCPU(0x%02x): 0x%08lx = vmread(0x%04lx)", vcpu->id, value, encoding);
	return value;
}

void vmcs_dump(VCPU *vcpu, int verbose)
{
	(void) vcpu;
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			unsigned long value;										\
			HALT_ON_ERRORCOND(__vmx_vmread(encoding, &value));			\
			vcpu->vmcs.name = value;									\
			if (!verbose) {												\
				break;													\
			}															\
			if (sizeof(vcpu->vmcs.name) == 4) {							\
				printf("\nCPU(0x%02x): vcpu->vmcs." #name "=0x%08lx",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 8) {					\
				printf("\nCPU(0x%02x): vcpu->vmcs." #name "=0x%016lx",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else {													\
				HALT_ON_ERRORCOND(0);									\
			}															\
		} while (0);
	#include <lhv-vmcs-template.h>
	#undef DECLARE_FIELD
}

void vmcs_load(VCPU *vcpu)
{
	(void) vcpu;
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			unsigned long value;										\
			HALT_ON_ERRORCOND(__vmx_vmread(encoding, &value));			\
			if (vcpu->vmcs.name != value) { \
				if (sizeof(vcpu->vmcs.name) == 4) {							\
					printf("\nCPU(0x%02x): O vcpu->vmcs." #name "=0x%08lx",	\
							vcpu->id, value);						\
					printf("\nCPU(0x%02x): L vcpu->vmcs." #name "=0x%08lx",	\
							vcpu->id, vcpu->vmcs.name);						\
				} else if (sizeof(vcpu->vmcs.name) == 8) {					\
					printf("\nCPU(0x%02x): O vcpu->vmcs." #name "=0x%016lx",	\
							vcpu->id, value);						\
					printf("\nCPU(0x%02x): L vcpu->vmcs." #name "=0x%016lx",	\
							vcpu->id, vcpu->vmcs.name);						\
				} else {													\
					HALT_ON_ERRORCOND(0);									\
				}															\
				{ \
					unsigned long value = vcpu->vmcs.name;						\
					HALT_ON_ERRORCOND(__vmx_vmwrite(encoding, value));			\
				} \
			} \
		} while (0);
	#include <lhv-vmcs-template.h>
	#undef DECLARE_FIELD
}
