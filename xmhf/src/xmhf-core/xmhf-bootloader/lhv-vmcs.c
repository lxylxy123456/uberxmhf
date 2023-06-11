/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			if ((encoding & 0x6000) == 0x0000) {						\
				vcpu->vmcs.name = __vmx_vmread16(encoding);				\
			} else if ((encoding & 0x6000) == 0x2000) {					\
				vcpu->vmcs.name = __vmx_vmread64(encoding);				\
			} else if ((encoding & 0x6000) == 0x4000) {					\
				vcpu->vmcs.name = __vmx_vmread32(encoding);				\
			} else {													\
				HALT_ON_ERRORCOND((encoding & 0x6000) == 0x6000);		\
				vcpu->vmcs.name = __vmx_vmreadNW(encoding);				\
			}															\
			if (!verbose) {												\
				break;													\
			}															\
			if (sizeof(vcpu->vmcs.name) == 4) {							\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%08x\n",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 8) {					\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%016lx\n",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 2) {					\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%04x\n",	\
						vcpu->id, (u32) vcpu->vmcs.name);				\
			} else {													\
				HALT_ON_ERRORCOND(0);									\
			}															\
		} while (0);
	#include <lhv-vmcs-template.h>
	#undef DECLARE_FIELD
}

void vmcs_load(VCPU *vcpu)
{
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			if ((encoding & 0x6000) == 0x0000) {						\
				__vmx_vmwrite16(encoding, vcpu->vmcs.name);				\
			} else if ((encoding & 0x6000) == 0x2000) {					\
				__vmx_vmwrite64(encoding, vcpu->vmcs.name);				\
			} else if ((encoding & 0x6000) == 0x4000) {					\
				__vmx_vmwrite32(encoding, vcpu->vmcs.name);				\
			} else {													\
				HALT_ON_ERRORCOND((encoding & 0x6000) == 0x6000);		\
				__vmx_vmwriteNW(encoding, vcpu->vmcs.name);				\
			}															\
		} while (0);
	#include <lhv-vmcs-template.h>
	#undef DECLARE_FIELD
}
