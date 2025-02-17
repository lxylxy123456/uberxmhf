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

/*-
 * Copyright (c) 2002 Mike Barcroft <mike@FreeBSD.org>
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      From: @(#)ansi.h        8.2 (Berkeley) 1/4/94
 *      From: @(#)types.h       8.3 (Berkeley) 1/5/94
 * $FreeBSD: stable/8/sys/i386/include/_types.h 199583 2009-11-20 15:27:52Z jhb $
 */

/**
 * Modified for XMHF to remove cdefs.h requirement.
 */

#ifndef _MACHINE__TYPES_H_
#define _MACHINE__TYPES_H_

#define __NO_STRICT_ALIGNMENT

/* number of bits per byte */
#define NBBY 8

/*
 * Basic types upon which most other types are built.
 */
typedef __signed char           __int8_t;
typedef unsigned char           __uint8_t;
typedef short                   __int16_t;
typedef unsigned short          __uint16_t;
typedef int                     __int32_t;
typedef unsigned int            __uint32_t;

/* LONGLONG */
typedef long long               __int64_t;
/* LONGLONG */
typedef unsigned long long      __uint64_t;

/*
 * Standard type definitions.
 */
typedef unsigned long   __clock_t;              /* clock()... */
typedef unsigned int    __cpumask_t;
typedef __int32_t       __critical_t;
typedef long double     __double_t;
typedef long double     __float_t;
typedef __int32_t       __intfptr_t;
typedef __int64_t       __intmax_t;
typedef __int32_t       __intptr_t;
typedef __int32_t       __int_fast8_t;
typedef __int32_t       __int_fast16_t;
typedef __int32_t       __int_fast32_t;
typedef __int64_t       __int_fast64_t;
typedef __int8_t        __int_least8_t;
typedef __int16_t       __int_least16_t;
typedef __int32_t       __int_least32_t;
typedef __int64_t       __int_least64_t;
typedef __int32_t       __ptrdiff_t;            /* ptr1 - ptr2 */
typedef __int32_t       __register_t;
typedef __int32_t       __segsz_t;              /* segment size (in pages) */
typedef __uint32_t      __size_t;               /* sizeof() */
typedef __int32_t       __ssize_t;              /* byte count or error */
typedef __int32_t       __time_t;               /* time()... */
typedef __uint32_t      __uintfptr_t;
typedef __uint64_t      __uintmax_t;
typedef __uint32_t      __uintptr_t;
typedef __uint32_t      __uint_fast8_t;
typedef __uint32_t      __uint_fast16_t;
typedef __uint32_t      __uint_fast32_t;
typedef __uint64_t      __uint_fast64_t;
typedef __uint8_t       __uint_least8_t;
typedef __uint16_t      __uint_least16_t;
typedef __uint32_t      __uint_least32_t;
typedef __uint64_t      __uint_least64_t;
typedef __uint32_t      __u_register_t;

typedef __uint32_t off_t;

/* varargs stuff removed */

#endif /* !_MACHINE__TYPES_H_ */
