# gnu-efi/: see README.md

CFLAGS = -Ignu-efi/inc
CFLAGS += -Wall -Werror
CFLAGS += -g
CFLAGS += -fpic -ffreestanding -fno-stack-protector -fno-stack-check
CFLAGS += -fshort-wchar -mno-red-zone -maccumulate-outgoing-args

LDFLAGS = -shared -Bsymbolic -Lgnu-efi/x86_64/lib -Lgnu-efi/x86_64/gnuefi
LDFLAGS += -Tgnu-efi/gnuefi/elf_x86_64_efi.lds
LDLIBS = -lgnuefi -lefi

OBJCOPY_FLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel
OBJCOPY_FLAGS += -j .rela -j .rel.* -j .rela.* -j .reloc
OBJCOPY_FLAGS += --target efi-app-x86_64 --subsystem=10

all: cdimage.iso

main.so: gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o main.o
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@
	chmod -x main.so

main.efi: main.so
	objcopy $(OBJCOPY_FLAGS) main.so main.efi
	chmod -x main.efi

fat.img: main.efi
	dd if=/dev/zero of=fat.img bs=1k count=1440
	mformat -i fat.img -f 1440 ::
	mmd -i fat.img ::/EFI
	mmd -i fat.img ::/EFI/BOOT
	mcopy -i fat.img main.efi ::/EFI/BOOT
	ln main.efi BOOTX64.EFI
	# If the following line is commented, need to run main.efi using EFI shell.
	# Otherwise, BOOTX64.EFI will run automatically.
	mcopy -i fat.img BOOTX64.EFI ::/EFI/BOOT
	rm -f BOOTX64.EFI

cdimage.iso: fat.img
	mkdir iso
	ln fat.img iso
	xorriso -as mkisofs -R -f -e fat.img -no-emul-boot -o cdimage.iso iso
	rm iso/fat.img
	rm -d iso

clean:
	rm -f main.o main.so main.efi fat.img cdimage.iso
	rm -rf iso/

