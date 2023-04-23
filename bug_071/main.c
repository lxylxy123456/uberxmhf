#include <efi.h>
#include <efilib.h>

void debug_nop(void)
{
	(void)debug_nop;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	InitializeLib(ImageHandle, SystemTable);

	Print(L"Hello, world!\n");
	Print(L"%p!\n", efi_main);

	/* https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB */
	{
		EFI_LOADED_IMAGE *loaded_image = NULL;
		EFI_STATUS status;
		status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3,
								   ImageHandle, &LoadedImageProtocol,
								   (void **)&loaded_image);
		if (EFI_ERROR(status)) {
			Print(L"handleprotocol: %r\n", status);
			// TODO
		}
		Print(L"Image base: 0x%lx\n", loaded_image->ImageBase);
	}

	/* https://www.rodsbooks.com/efi-programming/efi_services.html */
	uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, L"Hello\r\n");

	/* Look for serial device. */
	{
		//EFI_STATUS status;
		//status = uefi_call_wrapper(ST->BootServices->LocateHandle, 4,)
		//if (EFI_ERROR(status)) {
		//	Print(L"handleprotocol: %r\n", status);
		//	// TODO
		//}
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

