#include <xmhf.h>
#include <lhv.h>

// TODO: optimize by caching

void vmcs_vmwrite(VCPU *vcpu, ulong_t encoding, ulong_t value)
{
	(void) vcpu;
	// printf("CPU(0x%02x): vmwrite(0x%04lx, 0x%08lx)\n", vcpu->id, encoding, value);
	HALT_ON_ERRORCOND(__vmx_vmwrite(encoding, value));
}

void vmcs_vmwrite64(VCPU *vcpu, ulong_t encoding, u64 value)
{
	(void) vcpu;
	__vmx_vmwrite64(encoding, value);
}

ulong_t vmcs_vmread(VCPU *vcpu, ulong_t encoding)
{
	unsigned long value;
	(void) vcpu;
	HALT_ON_ERRORCOND(__vmx_vmread(encoding, &value));
	// printf("CPU(0x%02x): 0x%08lx = vmread(0x%04lx)\n", vcpu->id, value, encoding);
	return value;
}

u64 vmcs_vmread64(VCPU *vcpu, ulong_t encoding)
{
	(void) vcpu;
	return __vmx_vmread64(encoding);
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
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%08lx\n",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 8) {					\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%016lx\n",	\
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
			unsigned long value = vcpu->vmcs.name;						\
			HALT_ON_ERRORCOND(__vmx_vmwrite(encoding, value));			\
		} while (0);
	#include <lhv-vmcs-template.h>
	#undef DECLARE_FIELD
}
