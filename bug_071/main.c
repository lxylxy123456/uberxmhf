#include <efi.h>
#include <efilib.h>

#define CHK_EFI_ERROR(status) \
	do { \
		if (EFI_ERROR(status)) { \
			Print(L"CHK_EFI_ERROR at %d failed with %r\n", __LINE__, status); \
			while (1) { \
				asm volatile("hlt"); \
			} \
		} \
	} while(0)

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

	/* Look for serial device, from u-boot. */
	{
		EFI_STATUS status;
		EFI_SERIAL_IO_PROTOCOL *Interface = NULL;
		status = uefi_call_wrapper(ST->BootServices->LocateProtocol, 3
								   &SerialIoProtocol, NULL,
								   (void **)&Interface);
		CHK_EFI_ERROR(status);
	}

	/* Look for serial device. */
	{
		EFI_STATUS status;
		UINTN NoHandles;
		EFI_HANDLE *Buffer;
		status = uefi_call_wrapper(ST->BootServices->LocateHandleBuffer, 4,
								   ByProtocol, &SerialIoProtocol, NULL,
								   &NoHandles, &Buffer);
		CHK_EFI_ERROR(status);
		Print(L"NoHandles: 0x%ld\n", NoHandles);
		Print(L"Buffer: 0x%p\n", Buffer);

		for (UINTN i = 0; i < NoHandles; i++) {
			Print(L"Buffer[%ld]: 0x%p\n", i, Buffer[i]);
			debug_nop();
			{
				EFI_STATUS status;
				EFI_SERIAL_IO_PROTOCOL *Interface = NULL;
				status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3
										   Buffer[i], &SerialIoProtocol,
										   (void **)&Interface);
				CHK_EFI_ERROR(status);
				Print(L"\tInterface: %p\n", Interface);
			}
		}
	}

	/* Prevent exiting, useful if not using EFI shell. */
	if (1) {
		Print(L"Completed\n");
		while (1) {
			uefi_call_wrapper(BS->Stall, 1, 1000000);
			debug_nop();
		}
	}

	return EFI_SUCCESS;
}

