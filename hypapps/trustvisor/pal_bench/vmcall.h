#include <stdint.h>

__attribute__((always_inline))
inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	uint32_t a = 0x7a567254U, c = 0U;
#ifdef VMCALL_OFFSET
	c += VMCALL_OFFSET;
#endif
	asm volatile("cpuid" : "=a"(*(eax)), "=b"(*(ebx)), "=c"(*(ecx)),
					"=d"(*(edx)) : "0"(a), "2"(c));
}

__attribute__((always_inline))
inline uint64_t rdtsc(uint32_t *cpu)
{
	uint32_t eax = 0xf9, ebx = 0, ecx = 0, edx = 0;
#ifdef VMCALL_OFFSET
	eax += VMCALL_OFFSET;
#endif
	asm volatile ("vmcall" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
	*cpu = ecx;
	while (ebx != 0x64627374U) {
		asm volatile("pause");
	}
	// asm volatile("mfence; lfence; rdtsc; lfence" : "=a"(eax), "=d"(edx));
	return ((uint64_t)edx << 32) | eax;
}

static inline uintptr_t vmcall(uintptr_t eax, uintptr_t ecx, uintptr_t edx,
								uintptr_t esi, uintptr_t edi) {
#ifdef VMCALL_OFFSET
	eax += VMCALL_OFFSET;
#endif
	asm volatile ("vmcall\n\t" : "+a"(eax) : "c"(ecx), "d"(edx), "S"(esi),
					"D"(edi));
	return eax;
}

/* Return whether TrustVisor is present */
static inline int check_cpuid() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(&eax, &ebx, &ecx, &edx);
	if (eax == 0x7a767274U) {
		return 1;
	}
	return 0;
}
