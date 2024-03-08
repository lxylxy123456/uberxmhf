/*
 * The following license and copyright notice applies to all content in
 * this repository where some other license does not take precedence. In
 * particular, notices in sub-project directories and individual source
 * files take precedence over this file.
 *
 * See COPYING for more information.
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2023 Eric Li
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the copyright holder nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <stdio.h>

#include "vmcall.h"
#include "pal.h"
#include "caller.h"
#include "trustvisor.h"

void call_pal(uintptr_t a, uintptr_t b) {
	uintptr_t b2 = b;
	// Construct struct tv_pal_params
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	// Register scode
	void *entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 1);
	typeof(my_pal) *func = (typeof(my_pal) *)entry;
	// Call function
	printf("With PAL:\n");
	printf(" %p = *%p\n", (void *)b2, &b2);
	fflush(stdout);
	uintptr_t ret = func(a | PAL_FLAG_MASK, &b2);
	printf(" %p = my_pal(%p, %p)\n", (void *)ret, (void *)a, &b2);
	printf(" %p = *%p\n\n", (void *)b2, &b2);
	fflush(stdout);
	// Unregister scode
	unregister_pal(entry);
}

int main(int argc, char *argv[]) {
	uintptr_t a, b, b2;
	uintptr_t ret;
	if (!check_cpuid()) {
		printf("Error: TrustVisor not present according to CPUID\n");
		return 1;
	}
	assert(argc > 2);
	assert(sscanf(argv[1], "%p", (void **)&a) == 1);
	assert(sscanf(argv[2], "%p", (void **)&b) == 1);
	b2 = b;
	printf("Without PAL:\n");
	printf(" %p = *%p\n", (void *)b2, &b2);
	a &= ~PAL_FLAG_MASK;
	fflush(stdout);
	ret = my_pal(a, &b2);
	printf(" %p = my_pal(%p, %p)\n", (void *)ret, (void *)a, &b2);
	printf(" %p = *%p\n\n", (void *)b2, &b2);
	fflush(stdout);
	call_pal(a, b);
	return 0;
}
