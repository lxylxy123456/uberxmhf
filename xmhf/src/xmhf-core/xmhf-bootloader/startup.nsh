# Run XMHF
FS0:
cd EFI
cd BOOT
load init-x86-amd64.efi

# Sleep 5
stall 5000000

# Load Debian
FS1:
cd EFI
cd BOOT
BOOTX64.EFI

