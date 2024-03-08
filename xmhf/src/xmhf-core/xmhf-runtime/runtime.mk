#
# The following license and copyright notice applies to all content in
# this repository where some other license does not take precedence. In
# particular, notices in sub-project directories and individual source
# files take precedence over this file.
#
# See COPYING for more information.
#
# eXtensible, Modular Hypervisor Framework 64 (XMHF64)
# Copyright (c) 2023 Eric Li
# All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
#
# Neither the name of the copyright holder nor the names of
# its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

# Common makefile rules for components under xmhf-runtime
# author: Eric Li (xiaoyili@andrew.cmu.edu)
#
# Currently there are two subarch configurations:
#   * Boot loader is i386 BIOS booting, secure loader and runtime are also i386
#   * Boot loader is i386 BIOS booting, secure loader and runtime are amd64
#   * Boot loader is amd64 UEFI booting, secure loader and runtime are amd64
#
# In the first case, object files need to be only compiled once. In the second
# case, objects needed by the boot loader need to be copied in both i386 and
# amd64. In the third case, boot loader's object files need to be compiled
# differently (need GCC -fpic). So each component needs to declare whether a
# source is needed by the bootloader.
#
# These variables should have been already defined when including this file
#   * C_SOURCES: .c files needed by secure loader / runtime
#   * AS_SOURCES: .S files needed by secure loader / runtime
#   * C_SOURCES_BL: .c files needed by boot loader when runtime is amd64
#   * AS_SOURCES_BL: .S files needed by boot loader when runtime is amd64
#   * EXTRA_CLEAN: files to be cleaned other than OBJECTS and OBJECTS_BL
#
# This file will define these variables
#   * OBJECTS: .o files needed by secure loader / runtime
#   * OBJECTS_BL: .o files needed by boot loader when runtime is amd64
#
# This file will define these targets
#   * all: the default target, contains OBJECTS and OBJECTS_BL
#   * *.o: built for secure loader / runtime
#   * *.bl.o: built for boot loader when runtime is amd64
#   * clean: remove object files and EXTRA_CLEAN

_AS_OBJECTS = $(patsubst %.S, %.o, $(AS_SOURCES))
_C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
OBJECTS = $(_AS_OBJECTS) $(_C_OBJECTS)
OBJECTS_BL =

.PHONY: all
all: $(OBJECTS)

# Generate .d files
DEPFLAGS = -MMD

M_SOURCES = Makefile ../Makefile ../../Makefile

$(_AS_OBJECTS): %.o: %.S $(M_SOURCES)
	$(CC) -c $(ASFLAGS) $(DEPFLAGS) -o $@ $<
$(_C_OBJECTS): %.o: %.c $(M_SOURCES)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -o $@ $<

# If runtime is amd64, compile bootloader version of object files
ifeq ($(TARGET_SUBARCH), amd64)
_AS_OBJECTS_BL = $(patsubst %.S, %.bl.o, $(AS_SOURCES_BL))
_C_OBJECTS_BL = $(patsubst %.c, %.bl.o, $(C_SOURCES_BL))
OBJECTS_BL = $(_AS_OBJECTS_BL) $(_C_OBJECTS_BL)

all: $(OBJECTS_BL)

$(_AS_OBJECTS_BL): %.bl.o: %.S $(M_SOURCES)
	$(CC32) -c $(BASFLAGS) $(DEPFLAGS) -o $@ $<
$(_C_OBJECTS_BL): %.bl.o: %.c $(M_SOURCES)
	$(CC32) -c $(BCFLAGS) $(DEPFLAGS) -o $@ $<
endif

_DEP_FILES = $(patsubst %.o, %.d, $(OBJECTS) $(OBJECTS_BL))

-include $(_DEP_FILES)

.PHONY: clean
clean:
	$(RM) -rf $(OBJECTS) $(OBJECTS_BL) $(_DEP_FILES) $(EXTRA_CLEAN)

