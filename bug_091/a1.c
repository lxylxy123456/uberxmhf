// gcc a.c -O0 -o a.0 && gcc a.c -O3 -o a.3 && ./a.0 && echo SUCCESS && ./a.3

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static inline u32 __vmx_invvpid(int invalidation_type, u16 vpid, uintptr_t linearaddress){
	//return status (1 or 0)
	u32 status;

	//invvpid descriptor
	union {
		struct {
			u64 vpid : 16;
			u64 reserved : 48;
			u64 linearaddress;
		} fields;
		char raw[16];
	} invvpiddescriptor = { .fields = {vpid, 0, linearaddress} };

	//issue invvpid instruction
	//note: GCC does not seem to support this instruction directly
	//so we encode it as hex
	__asm__ __volatile__("cmpl (%%rax), %%ecx;"
	                     // " .byte 0x66, 0x0f, 0x38, 0x81, 0x08 \r\n"
	                     "jnz 1f;"
	                     "cmpl (%%rax), %%ecx;"
	                     "movl $1, %%eax \r\n"
	                     "ja 1f \r\n"
	                     "movl $0, %%eax \r\n"
	                     "1: movl %%eax, %0 \r\n"
	  : "=m" (status)
	  : "a"(invvpiddescriptor.raw), "c"(invalidation_type)
	  : "cc", "memory");

	return status;
}

void func(void)
{
	assert(!__vmx_invvpid(0, 0, 0x0));
	assert(__vmx_invvpid(0, 3, 0x0));
}

int main(void)
{
	func();
}
