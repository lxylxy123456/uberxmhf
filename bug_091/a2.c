// gcc a.c -O0 -o a.0 && gcc a.c -O3 -o a.3 && ./a.0 && echo SUCCESS && ./a.3

#include <stdio.h>

static inline void __vmx_invvpid(long vpid){
	long a = vpid;
	asm volatile(
		"cmpl (%%rax), %%ecx;"
		"movl $0, %%eax"
		: : "a"(&a) : "cc", "memory");
}

int main(void)
{
	__vmx_invvpid(0);
	__vmx_invvpid(3);
}
