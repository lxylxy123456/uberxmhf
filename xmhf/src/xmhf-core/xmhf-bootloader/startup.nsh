# Run XMHF
FS0:
cd EFI
cd BOOT
load efi.efi

# Sleep 5
stall 5000000

# Load Debian
FS1:
cd EFI
cd BOOT
BOOTX64.EFI

