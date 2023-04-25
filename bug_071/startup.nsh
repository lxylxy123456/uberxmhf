# Run EFI application
FS0:
cd EFI
cd BOOT
load main.efi

# Sleep 5
stall 1000000
stall 1000000
stall 1000000
stall 1000000
stall 1000000

# Load Debian
FS1:
cd EFI
cd BOOT
BOOTX64.EFI

