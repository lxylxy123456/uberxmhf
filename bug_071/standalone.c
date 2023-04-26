#include <efi.h>
#include <efilib.h>

#define SERIAL_PORT (0x3f8)

static inline void outb(uint8_t value, uint16_t port)
{
	__asm__ __volatile__ ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

static inline uint8_t inb(uint16_t port)
{
	unsigned char _v;
	__asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (port));
	return _v;
}

static void dbg_x86_uart_putc_bare(char ch){
	//wait for xmit hold register to be empty
	while ( ! (inb(SERIAL_PORT + 0x5) & 0x20) ) {
		asm volatile("pause");
	}

	//write the character
	outb(ch, SERIAL_PORT);
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	for (char *s = "Hello, serial!\n"; *s != '\0'; s++) {
		dbg_x86_uart_putc_bare(*s);
	}
	return EFI_SUCCESS;
}

