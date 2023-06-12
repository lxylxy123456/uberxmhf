__attribute__((noreturn))
static inline void cpu_halt(void)
{
	while (1) {
		asm volatile("hlt");
	}
}

static inline void cpu_relax(void)
{
	asm volatile("pause");
}

static inline u8 inb(u16 port)
{
	u8 ans;
	asm volatile("inb %1, %0" : "=a"(ans) : "d"(port));
	return ans;
}

static inline void outb(u16 port, u8 val)
{
	asm volatile("outb %1, %0" : : "d"(port), "a"(val));
}

