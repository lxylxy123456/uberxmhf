/*
gcc -c -O3 -g -m32 -o /tmp/a.o a1.c
*/

static inline int __vmx_vmwrite2(unsigned long encoding, unsigned long value){
  int status;
  __asm__("vmwrite %2, %1 \r\n"
          "movl $1, %0 \r\n"
	  : "=g"(status)
	  : "r"(encoding), "g"(value)
      : "cc"
    );
	return status;
}

void f(void)
{
	unsigned long control_vpid = 0;
	while (__vmx_vmwrite2(0x0000, control_vpid));
}

