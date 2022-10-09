/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
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
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
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
 *
 * @XMHF_LICENSE_HEADER_END@
 */

#include <tomcrypt.h>
#include <euchk.h>

#include <sha1.h>

void hashandprint(const char* prefix, const u8 *bytes, size_t len) {
    u8 digest[SHA1_DIGEST_LENGTH];

#ifdef __AMD64__
    printf("hashandprint: processing 0x%016x bytes at addr %016x\n", len, (u64)bytes);
#elif defined(__I386__)
    printf("hashandprint: processing 0x%08x bytes at addr 0x%08x\n", len, (u32)bytes);
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

    EU_VERIFYN( sha1_buffer(bytes, len, digest));

    printf("%s: %*D\n", prefix, SHA1_DIGEST_LENGTH, digest, " ");
    /* print_hex( prefix, digest, SHA1_DIGEST_LENGTH); */

    /* Simulate PCR 17 value on AMD processor */
    /* if(len == 0x10000) { */
        /* u8 zeros[SHA1_DIGEST_LENGTH]; */
        /* u8 pcr17[SHA1_DIGEST_LENGTH]; */
        /* memset(zeros, 0, SHA1_DIGEST_LENGTH); */

        /* SHA1_Init(&ctx); */
        /* SHA1_Update(&ctx, zeros, SHA1_DIGEST_LENGTH); */
        /* SHA1_Update(&ctx, digest, SHA1_DIGEST_LENGTH); */
        /* SHA1_Final(pcr17, &ctx); */

        /* print_hex("[AMD] Expected PCR-17: ", pcr17, SHA1_DIGEST_LENGTH); */
    /* }     */
}
