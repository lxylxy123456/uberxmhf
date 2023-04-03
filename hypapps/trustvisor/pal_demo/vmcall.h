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

#define INLINE __attribute__((always_inline)) inline

INLINE int get_edi(void) {
	int ans;
	asm volatile ("movl %%edi, %0" : "=a"(ans));
	return ans;
}

INLINE void set_edi(int val) {
	asm volatile ("movl %0, %%edi" : : "a"(val));
}

INLINE int get_edx(void) {
	int ans;
	asm volatile ("movl %%edx, %0" : "=a"(ans));
	return ans;
}

INLINE void set_edx(int val) {
	asm volatile ("movl %0, %%edx" : : "a"(val));
}

INLINE int fstp(void) {
	int ans;
	asm volatile ("fstps %0" : "=m"(ans));
	return ans;
}

INLINE void fld(int val) {
	asm volatile ("flds %0" : : "m"(val));
}

INLINE int get_xmm(void) {
	int ans;
	asm volatile ("movd %%xmm0, %0" : "=a"(ans));
	return ans;
}

INLINE void set_xmm(int val) {
	asm volatile ("movd %0, %%xmm0" : : "a"(val));
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
