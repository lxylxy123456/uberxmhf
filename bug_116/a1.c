#include <stdio.h>

#define INLINE
// #define INLINE __attribute__((always_inline)) inline

INLINE int get_edi(void) {
	int ans;
	asm volatile ("movl %%edi, %0" : "=a"(ans));
	return ans;
}

INLINE void set_edi(int val) {
	asm volatile ("movl %0, %%edi" : : "a"(val));
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

int main(void) {
	int e1, e2, e3;
	set_edi(34);
	e1 = get_edi();
	set_edi(35);
	e2 = get_edi();
	set_edi(36);
	e3 = get_edi();
	printf("%d %d %d\n", e1, e2, e3);
	fld(12);
	printf("%d ", fstp());
	fld(13);
	printf("%d\n", fstp());
	set_xmm(96);
	printf("%d ", get_xmm());
	set_xmm(97);
	printf("%d\n", get_xmm());
	return 0;
}

