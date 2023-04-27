# makefile for "init"
# author: amit vasudevan (amitvasudevan@acm.org)

CFLAGS := $(BCFLAGS)
ASFLAGS := $(BASFLAGS)
CCLIB := $(BCCLIB)

# source files
AS_SOURCES =
# TODO: begin removed for UEFI
#AS_SOURCES += header.S
# TODO: end removed for UEFI
AS_SOURCES += initsup.S
C_SOURCES = init.c smp.c cmdline.c
ifeq ($(DRT), y)
C_SOURCES += txt.c txt_acmod.c txt_heap.c txt_hash.c
endif

OBJECTS = $(patsubst %.S, %.o, $(AS_SOURCES))
OBJECTS += $(patsubst %.c, %.o, $(C_SOURCES))

# TODO: begin added for UEFI
OBJECTS += $(GNUEFI_BUILD)/x86_64/gnuefi/crt0-efi-x86_64.o
# TODO: ideally should be C_SOURCES
OBJECTS += efi.o
# TODO: end added for UEFI

OBJECTS_PRECOMPILED = ../xmhf-runtime/xmhf-debug/lib.a

OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-tpm/tpm-interface.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-tpm/arch/x86/tpm-x86.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-tpm/arch/x86/svm/tpm-x86svm.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-tpm/arch/x86/vmx/tpm-x86vmx.o

OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pci.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-acpi.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pit.o
# TODO: begin modified for UEFI
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-i386-smplock.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-amd64-smplock.o
# TODO: end modified for UEFI
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-cpu.o

ifeq ($(DRT), y)
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-common.o
OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs-bootloader.o
endif

# FIXME: This is overly general; init/ doesn't need all of these
CFLAGS += $(ADDL_INCLUDES_BOOTLOADER)

# separate from OBJECTS_PRECOMPILED becasue needs to come after libs on link line
OBJECTS_PRECOMPILED_LIBBACKENDS = ../xmhf-runtime/xmhf-xmhfcbackend/xmhfc-putchar.o

# TODO: begin added for UEFI
OBJECTS_PRECOMPILED_LIBBACKENDS += ../xmhf-runtime/xmhf-debug/lib.a
# TODO: end added for UEFI

I_SOURCES =  $(wildcard $(INCLUDEDIR)/*.h)


# Bootloader is always i386. If runtime is amd64, bootloader need to switch to
# i386 version of precompiled object files.
ifeq ($(TARGET_SUBARCH), amd64)
OBJECTS_PRECOMPILED := $(patsubst %.o, %.i386.o, $(OBJECTS_PRECOMPILED))
OBJECTS_PRECOMPILED := $(patsubst %.a, %.i386.a, $(OBJECTS_PRECOMPILED))
OBJECTS_PRECOMPILED_LIBBACKENDS := $(patsubst %.o, %.i386.o, $(OBJECTS_PRECOMPILED_LIBBACKENDS))
# TODO: begin added for UEFI
OBJECTS_PRECOMPILED_LIBBACKENDS := $(patsubst %.a, %.i386.a, $(OBJECTS_PRECOMPILED_LIBBACKENDS))
# TODO: end added for UEFI
endif


# NOTE: THIS IS THE init MODULE.  IT IS COMPLETELY UNTRUSTED.  THESE
# VALUES ARE HERE SIMPLY TO AID IN DEVELOPMENT AND DEBUGGING, TO STOP
# EARLY IN THE EVENT OF MISTAKES.  DO NOT RELY ON THEM!

# RUNTIME_INTEGRITY_HASH should be set by parent Makefile
ifdef RUNTIME_INTEGRITY_HASH
CFLAGS += -D___RUNTIME_INTEGRITY_HASH___=\"$(RUNTIME_INTEGRITY_HASH)\"
endif
# SLABOVE64K_INTEGRITY_HASH should be set by parent Makefile
ifdef SLABOVE64K_INTEGRITY_HASH
CFLAGS += -D___SLABOVE64K_INTEGRITY_HASH___=\"$(SLABOVE64K_INTEGRITY_HASH)\"
endif
# SLBELOW64K_INTEGRITY_HASH should be set by parent Makefile
ifdef SLBELOW64K_INTEGRITY_HASH
CFLAGS += -D___SLBELOW64K_INTEGRITY_HASH___=\"$(SLBELOW64K_INTEGRITY_HASH)\"
endif


# targets
.PHONY: all
# TODO: begin modified for UEFI
#all: init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin
all:
	touch init-x86-amd64.bin
# TODO: end modified for UEFI

# FIXME: ADDL_LIBS is overly general; init/ doesn't need all of them
init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin: $(OBJECTS) $(OBJECTS_PRECOMPILED) init.lds.S
	$(LD) $(LDFLAGS) -T init.lds.S -o init.exe $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS) -L$(CCLIB) -lgcc
	$(CP) init.exe init_syms.exe
	$(STRIP) -s init.exe
	$(OBJCOPY) --output-format=binary init.exe init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin

# TODO: begin added for UEFI
EFI_CFLAGS = -I$(GNUEFI_SRC)/inc
EFI_CFLAGS += -Wall -Werror
EFI_CFLAGS += -g
EFI_CFLAGS += -fpic -ffreestanding -fno-stack-protector -fno-stack-check
EFI_CFLAGS += -fshort-wchar -mno-red-zone -maccumulate-outgoing-args

EFI_LDFLAGS = -shared -Bsymbolic -L$(GNUEFI_BUILD)/x86_64/lib
EFI_LDFLAGS += -L$(GNUEFI_BUILD)/x86_64/gnuefi
EFI_LDLIBS = -lgnuefi -lefi

OBJCOPY_FLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel
OBJCOPY_FLAGS += -j .rela -j .rel.* -j .rela.* -j .reloc
OBJCOPY_FLAGS += --target efi-app-x86_64 --subsystem=12

# TODO
all: efi.efi fat.img cdimage.iso

# TODO: add make clean

# TODO: rename efi -> init
efi.o: efi.c
	$(CC) $(EFI_CFLAGS) -c $< -o $@

efi.so: $(OBJECTS) $(OBJECTS_PRECOMPILED) $(GNUEFI_SRC)/gnuefi/elf_x86_64_efi.lds
	$(LD) $(EFI_LDFLAGS) -T$(GNUEFI_SRC)/gnuefi/elf_x86_64_efi.lds $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS) $(EFI_LDLIBS) -o $@
	chmod -x $@

efi.efi: efi.so
	objcopy $(OBJCOPY_FLAGS) efi.so efi.efi
	chmod -x $@

fat.img: efi.efi startup.nsh
	dd if=/dev/zero of=fat.img bs=1k count=1440
	mformat -i fat.img -f 1440 ::
	mmd -i fat.img ::/EFI
	mmd -i fat.img ::/EFI/BOOT
	for i in $^; do mcopy -i fat.img "$$i" ::/EFI/BOOT; done
	# If BOOTX64.EFI is present and startup.nsh is not, will run automatically.

cdimage.iso: fat.img
	mkdir iso
	ln fat.img iso
	xorriso -as mkisofs -R -f -e fat.img -no-emul-boot -o cdimage.iso iso
	rm iso/fat.img
	rm -d iso
# TODO: end added for UEFI

.PHONY: clean
clean:
	$(RM) -rf *.o
	$(RM) -rf *.exe
	$(RM) -rf *.bin
	$(RM) -rf *.gz

