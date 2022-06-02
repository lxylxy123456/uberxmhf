/*
gcc -c -fno-builtin -fno-common -fno-strict-aliasing -iwithprefix include -fno-stack-protector -Wstrict-prototypes -Wdeclaration-after-statement -Wno-pointer-arith -Wextra -Wfloat-equal -Werror -Wsign-compare -Wno-bad-function-cast -Wall -Waggregate-return -Winline -march=k8 -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-avx -mno-aes -mno-pclmul -mno-sse4a -mno-3dnow -mno-popcnt -mno-abm -nostdinc -pipe -Wno-address-of-packed-member -fno-pie -fno-pic -mno-red-zone -O3 -Wno-stringop-overflow -g -m32 -MMD -o /tmp/a.o a.c
*/

typedef unsigned short u16;
typedef unsigned int u32;

#define HALT_ON_ERRORCOND2(_p) \
    do { \
        if ( !(_p) ) { \
            while (1); \
        } \
    } while (0)

static inline u32 __vmx_vmwrite2(unsigned long encoding, unsigned long value){
  u32 status;
  __asm__("vmwrite %2, %1 \r\n"
          "jbe 1f \r\n"
          "movl $1, %0 \r\n"
          "jmp 2f \r\n"
          "1: movl $0, %0 \r\n"
          "2: \r\n"
	  : "=g"(status)
	  : "r"(encoding), "g"(value)
      : "cc"
    );
	return status;
}

void f(void)
{
	u16 control_vpid = 0;
	HALT_ON_ERRORCOND2(__vmx_vmwrite2(0x0000, control_vpid));
}

