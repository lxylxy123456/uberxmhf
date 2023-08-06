# top-level makefile for XMHF x86 platform 
# author: amit vasudevan (amitvasudevan@acm.org)

# e.g. init-x86-i386.bin or init-x86-amd64.bin
INIT_BIN := init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).bin
INIT_EFI := init-$(TARGET_HWPLATFORM)-$(TARGET_SUBARCH).efi

#-----build rules
.PHONY: all
all: bootloader copy

.PHONY: copy
ifeq ($(TARGET_UEFI), y)
copy: bootloader
	$(CP) ./xmhf-bootloader/$(INIT_EFI) $(HYPOUTDIR)/$(INIT_EFI)
else
copy: bootloader
	$(CP) ./xmhf-bootloader/$(INIT_BIN) $(HYPOUTDIR)/$(INIT_BIN)
endif

.PHONY: bootloader
bootloader:
	cd xmhf-bootloader && $(MAKE) -w all


# cleanup rules
#.PHONY: clean init-late-clean
.PHONY: clean 
clean: 
	cd xmhf-bootloader && $(MAKE) -w clean
	rm -rf $(APPOBJECTSDIR)
	$(RM) -rf $(HYPOUTDIR)/$(INIT_BIN)
	$(RM) -rf $(HYPOUTDIR)/$(INIT_EFI)


.PHONY: install-dev
install-dev:
	# Nothing to do here

.PHONY: verify
verify:
	cd verification/ && $(MAKE) -w verify
	
.PHONY: verifyinit
verifyinit:
	cd verification/ && $(MAKE) -w verifyinit

.PHONY: verifyall
verifyall:
	cd verification/ && $(MAKE) -w verifyall
