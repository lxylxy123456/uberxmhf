# makefile for "init"
# author: amit vasudevan (amitvasudevan@acm.org)

CFLAGS := $(BCFLAGS)
ASFLAGS := $(BASFLAGS)
CCLIB := $(BCCLIB)

# source files
ifeq ($(TARGET_UEFI), y)
AS_SOURCES = efi2init.S initsup_uefi.S
else
AS_SOURCES = header.S initsup_bios.S
endif
C_SOURCES = init.c smp.c cmdline.c

# LHV files
AS_SOURCES += lhv-asm.S
AS_SOURCES += lhv-guest-asm.S
AS_SOURCES += lhv-user-asm.S

C_SOURCES += lhv.c
C_SOURCES += lhv-console.c
C_SOURCES += lhv-timer.c
C_SOURCES += lhv-pic.c
C_SOURCES += lhv-keyboard.c
C_SOURCES += lhv-vmx.c
C_SOURCES += lhv-ept.c
C_SOURCES += lhv-vmcs.c
C_SOURCES += lhv-guest.c
C_SOURCES += lhv-user.c
C_SOURCES += lhv-mouse.c

AS_SOURCES += spinlock.S

C_SOURCES += debug.c
C_SOURCES += debug-uart.c
C_SOURCES += debug-vga.c

OBJECTS = $(patsubst %.S, %.o, $(AS_SOURCES))
OBJECTS += $(patsubst %.c, %.o, $(C_SOURCES))

ifeq ($(TARGET_UEFI), y)
OBJECTS += $(GNUEFI_BUILD)/x86_64/gnuefi/crt0-efi-x86_64.o
# Note: efi.o is compiled differently with other object files
OBJECTS += efi.o
endif

#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pci.o
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-acpi.o
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pit.o
#ifeq ($(TARGET_UEFI), y)
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-amd64-smplock.o
#else
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-i386-smplock.o
#endif
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.o
#OBJECTS_PRECOMPILED += ../xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-cpu.o

# FIXME: This is overly general; init/ doesn't need all of these
CFLAGS += $(ADDL_INCLUDES_BOOTLOADER)

# separate from OBJECTS_PRECOMPILED becasue needs to come after libs on link line
#OBJECTS_PRECOMPILED_LIBBACKENDS = ../xmhf-runtime/xmhf-xmhfcbackend/xmhfc-putchar.o

I_SOURCES =  $(wildcard $(INCLUDEDIR)/*.h)


# If runtime is amd64, bootloader need to switch to precompiled object files
# with special CFLAGS.
ifeq ($(TARGET_SUBARCH), amd64)
OBJECTS_PRECOMPILED := $(patsubst %.o, %.bl.o, $(OBJECTS_PRECOMPILED))
OBJECTS_PRECOMPILED := $(patsubst %.a, %.bl.a, $(OBJECTS_PRECOMPILED))
OBJECTS_PRECOMPILED_LIBBACKENDS := $(patsubst %.o, %.bl.o, $(OBJECTS_PRECOMPILED_LIBBACKENDS))
OBJECTS_PRECOMPILED_LIBBACKENDS := $(patsubst %.a, %.bl.a, $(OBJECTS_PRECOMPILED_LIBBACKENDS))
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
ifeq ($(TARGET_UEFI), y)
all: init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).efi
else
all: init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin
endif

# FIXME: ADDL_LIBS is overly general; init/ doesn't need all of them
init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin: $(OBJECTS) $(OBJECTS_PRECOMPILED) init.lds.S
	$(LD) $(LDFLAGS) -T init.lds.S -o init.exe $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS) -L$(CCLIB) -lgcc
	$(CP) init.exe init_syms.exe
	$(STRIP) -s init.exe
	$(OBJCOPY) --output-format=binary init.exe init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin

ifeq ($(TARGET_UEFI), y)

# CFLAGS from gnu-efi tutorial
EFI_CFLAGS = -I$(GNUEFI_SRC)/inc
EFI_CFLAGS += -Wall -Werror
EFI_CFLAGS += -g
EFI_CFLAGS += -fpic -ffreestanding -fno-stack-protector -fno-stack-check
EFI_CFLAGS += -fshort-wchar -mno-red-zone -maccumulate-outgoing-args
EFI_CFLAGS += $(BCFLAGS)
EFI_CFLAGS += -Wno-strict-prototypes

# LD flags from gnu-efi tutorial
EFI_LDFLAGS = -shared -Bsymbolic -L$(GNUEFI_BUILD)/x86_64/lib
EFI_LDFLAGS += -L$(GNUEFI_BUILD)/x86_64/gnuefi
EFI_LDLIBS = -lgnuefi -lefi

# OBJCOPY flags from gnu-efi tutorial
OBJCOPY_FLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel
OBJCOPY_FLAGS += -j .rela -j .rel.* -j .rela.* -j .reloc
OBJCOPY_FLAGS += --target efi-app-x86_64 --subsystem=12

efi.o: efi.c
	$(CC) $(EFI_CFLAGS) -c $< -o $@

init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).so: $(OBJECTS) $(OBJECTS_PRECOMPILED) $(GNUEFI_SRC)/gnuefi/elf_x86_64_efi.lds
	$(LD) $(EFI_LDFLAGS) -T$(GNUEFI_SRC)/gnuefi/elf_x86_64_efi.lds $(OBJECTS) $(OBJECTS_PRECOMPILED) $(ADDL_LIBS_BOOTLOADER) $(OBJECTS_PRECOMPILED_LIBBACKENDS) $(EFI_LDLIBS) -o $@
	chmod -x $@

init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).efi: init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).so
	objcopy $(OBJCOPY_FLAGS) $^ $@
	chmod -x $@

endif

.PHONY: clean
clean:
	$(RM) -rf *.o
	$(RM) -rf *.exe
	$(RM) -rf *.bin
	$(RM) -rf *.gz
	$(RM) -rf *.so
	$(RM) -rf *.efi

