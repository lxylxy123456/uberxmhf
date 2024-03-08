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

#include <stdint.h>

#define PAL_FLAG_MASK ((uintptr_t)0x80000000U)

extern void begin_pal_c();
extern uintptr_t my_pal(uintptr_t arg1, uintptr_t *arg2);
extern uintptr_t pal_10_int(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
							uintptr_t arg3, uintptr_t arg4, uintptr_t arg5,
							uintptr_t arg6, uintptr_t arg7, uintptr_t arg8,
							uintptr_t arg9);
extern uintptr_t pal_10_ptr(uintptr_t *arg0, uintptr_t *arg1, uintptr_t *arg2,
							uintptr_t *arg3, uintptr_t *arg4, uintptr_t *arg5,
							uintptr_t *arg6, uintptr_t *arg7, uintptr_t *arg8,
							uintptr_t *arg9);
extern uintptr_t pal_5_ptr(uintptr_t size0, uintptr_t *ptr0, uintptr_t size1,
							uintptr_t *ptr1, uintptr_t size2, uintptr_t *ptr2,
							uintptr_t size3, uintptr_t *ptr3, uintptr_t size4,
							uintptr_t *ptr4);
extern void end_pal_c();
