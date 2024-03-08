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

# Compile amd64 pal_demo for Linux on amd64 Linux:
#   make
# Compile i386 pal_demo for Linux on amd64 Debian:
#   make CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld
# Compile i386 pal_demo for Linux on amd64 Fedora:
#   make I386=y
# Compile i386 pal_demo for Windows on Linux:
#   make CC=i686-w64-mingw32-gcc LD=i686-w64-mingw32-ld
# Compile amd64 pal_demo for Windows on Linux:
#   make CC=x86_64-w64-mingw32-gcc LD=x86_64-w64-mingw32-ld

# -fno-stack-protector: hint from https://stackoverflow.com/questions/2340259/
CFLAGS := -g -I../src/include -Wall -Werror -fno-stack-protector

BINS = main test test_args
PAL_OBJS = pal.o caller.o
VMCALL_OFFSET = 0U

ifeq ($(WINDOWS), y)
	CFLAGS += -DWINDOWS
endif
ifeq ($(I386), y)
	CFLAGS += -m32
	LDFLAGS += -m32
endif
CFLAGS += -DVMCALL_OFFSET=$(VMCALL_OFFSET)

ifeq ($(CC), x86_64-w64-mingw32-gcc)
PAL_OBJS += translate.o
CFLAGS += -DTRANSLATE
endif

all: $(BINS)

main: main.o $(PAL_OBJS)
test_args: test_args.o $(PAL_OBJS)
test: test.c

pal.o: pal.c
	$(CC) $(CFLAGS) -fno-pic -c -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o $(patsubst %,%.exe,$(BINS)) $(BINS)

