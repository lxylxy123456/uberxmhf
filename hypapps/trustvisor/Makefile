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

# export EMHFCOREDIR := $(CURDIR)/../../../emhf/trunk/code/$(EMHFROOTDIR)/x86
# export EMHF_INCLUDEDIR = $(EMHFCOREDIR)/include

SRCDIR := $(dir $(lastword $(MAKEFILE_LIST)))
SRCDIR := $(realpath $(SRCDIR))

# TODO: Control via configuration
export THE_ARCHIVE = xmhfhypapp.a

export APP_INCLUDEDIR = $(SRCDIR)/src/include
export APPOBJECTSDIR = $(SRCDIR)/src/objects

#-----basic flags for compiling
CFLAGS += -I$(APP_INCLUDEDIR)

# app-specific configuration options
export LDN_TV_INTEGRATION := n

#lockdown-trustvisor integration demo
ifeq ($(LDN_TV_INTEGRATION), y)
  CFLAGS += -D__LDN_TV_INTEGRATION__
endif

export CFLAGS
export ASFLAGS

.PHONY: all
all: config.status
	# make app components
	mkdir -p $(APPOBJECTSDIR)
	cd src && $(MAKE) -w all

# There is no binary installation; only the EMHF core
# produces a complete binary of some sort.  All libraries
# are at the root level as well.
.PHONY: install
install: install-dev

# install development components
.PHONY: install-dev
install-dev:
	install -d $(DESTDIR)$(pkgconfigdir)
	install --mode=644 trustvisor.pc $(DESTDIR)$(pkgconfigdir)

	install -d $(DESTDIR)$(includedir)/trustvisor
	install $(SRCDIR)/src/include/trustvisor.h $(DESTDIR)$(includedir)/trustvisor
	install $(LIBBAREMETAL_SRC)/libtv_utpm/include/tv_utpm.h $(DESTDIR)$(includedir)/trustvisor

.PHONY: test
test:
	cd test && $(MAKE) -w all

.PHONY: clean
clean: 
	cd src && $(MAKE) -w clean
	rm -rf $(APPOBJECTSDIR) $(THE_ARCHIVE)
	cd test && $(MAKE) -w clean
	rm -f *.bin
	rm -f *.bin.gz

.PHONY: distclean
distclean: clean
	$(RM) config.log config.status
	$(RM) -rf trustvisor.pc

config.status: configure
	./configure

configure: configure.ac
	./autogen.sh
