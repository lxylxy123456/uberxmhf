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

static inline ulong_t read_cr0(void)
{
	ulong_t ans;
	asm volatile("mov %%cr0, %0" : "=r"(ans));
	return ans;
}

static inline void write_cr0(ulong_t val)
{
	asm volatile("mov %0, %%cr0" : : "r"(val));
}

