#include <efi.h>
#include <efilib.h>
#include <wchar.h>

#define CHK_EFI_ERROR(status) \
	do { \
		if (EFI_ERROR(status)) { \
			Print(L"CHK_EFI_ERROR at %d failed with %r\n", __LINE__, status); \
			while (1) { \
				asm volatile("hlt"); \
			} \
		} \
	} while(0)

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

void debug_nop(void)
{
	(void)debug_nop;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	InitializeLib(ImageHandle, SystemTable);

	Print(L"Hello, world from %p!\n", efi_main);

	for (char *s = "Hello, serial!\n"; *s != '\0'; s++) {
		dbg_x86_uart_putc_bare(*s);
	}

	/* https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB */
	{
		EFI_LOADED_IMAGE *loaded_image = NULL;
		EFI_STATUS status;
		status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3,
								   ImageHandle, &LoadedImageProtocol,
								   (void **)&loaded_image);
		CHK_EFI_ERROR(status);
		Print(L"Image base: 0x%lx\n", loaded_image->ImageBase);
	}

	/* https://www.rodsbooks.com/efi-programming/efi_services.html */
	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, L"Hello\r\n");

	/* Look for serial device (use LocateProtocol to only look for one). */
	{
		EFI_STATUS status;
		UINTN NoHandles;
		EFI_HANDLE *Buffer;
		/* Also: SerialIoProtocol */
		EFI_GUID *Protocol = &TextOutProtocol;
		status = uefi_call_wrapper(ST->BootServices->LocateHandleBuffer, 5,
								   ByProtocol, Protocol, NULL, &NoHandles,
								   &Buffer);
		CHK_EFI_ERROR(status);
		Print(L"NoHandles: 0x%ld\n", NoHandles);
		Print(L"Buffer: 0x%p\n", Buffer);

		for (UINTN i = 0; i < NoHandles; i++) {
			Print(L"Buffer[%ld]: 0x%p\n", i, Buffer[i]);
			EFI_SIMPLE_TEXT_OUT_PROTOCOL *Interface = NULL;
			{
				EFI_STATUS status;
				status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3,
										   Buffer[i], Protocol,
										   (void **)&Interface);
				CHK_EFI_ERROR(status);
				Print(L"    Interface: %p\n", Interface);
			}
			{
				EFI_STATUS status;
				status = uefi_call_wrapper(Interface->OutputString, 2,
										   Interface, L"    Hello\r\n");
				CHK_EFI_ERROR(status);
			}
		}
	}

	/* Allocate memory */
	{
		EFI_STATUS status;
		EFI_PHYSICAL_ADDRESS addr = 0x10000000;
		status = uefi_call_wrapper(ST->BootServices->AllocatePages, 4,
								   AllocateAddress,
								   EfiRuntimeServicesData,
								   32768,
								   &addr);
		CHK_EFI_ERROR(status);
		Print(L"Allocated: %p\n", addr);
	}

	/* Prevent exiting, useful if not using EFI shell. */
	if (0) {
		Print(L"Completed\n");
		while (1) {
			uefi_call_wrapper(BS->Stall, 1, 1000000);
			debug_nop();
		}
	}

	return EFI_SUCCESS;
}

