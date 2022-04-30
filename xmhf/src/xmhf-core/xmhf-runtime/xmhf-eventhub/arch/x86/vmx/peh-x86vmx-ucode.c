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

// peh-x86vmx-ucode.c
// Handle Intel microcode update
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>

#include <hptw.h>
#include <hpt_emhf.h>

/*
 * Maximum supported microcode size. For some CPUs this may need to be much
 * larger (e.g. > 256K).
 */
#define UCODE_TOTAL_SIZE_MAX (PAGE_SIZE_4K)

/* Space to temporarily copy microcode update (prevent TOCTOU attack) */
static u8 ucode_copy_area[MAX_VCPU_ENTRIES * UCODE_TOTAL_SIZE_MAX]
__attribute__(( section(".bss.palign_data") ));

typedef struct __attribute__ ((packed)) {
	u32 header_version;
	u32 update_version;
	u32 date;
	u32 processor_signature;
	u32 checksum;
	u32 loader_version;
	u32 processor_flags;
	u32 data_size;
	u32 total_size;
	u32 reserved[3];
	u8 update_data[0];
} intel_ucode_update_t;

typedef struct __attribute__ ((packed)) {
	u32 processor_signature;
	u32 processor_flags;
	u32 checksum;
} intel_ucode_ext_sign_t;

typedef struct __attribute__ ((packed)) {
	u32 extended_signature_count;
	u32 extended_processor_signature_table_checksum;
	u32 reserved[3];
	intel_ucode_ext_sign_t signatures[0];
} intel_ucode_ext_sign_table_t;

typedef struct {
	hptw_ctx_t guest_ctx;
	hptw_ctx_t host_ctx;
} ucode_hptw_ctx_pair_t;

unsigned char ucode_recognized_sha1s[][SHA_DIGEST_LENGTH] = {
	{0x93, 0x0e, 0xff, 0x4f, 0xc2, 0x51, 0x19, 0xda, 0xb5, 0x89, 0xe5, 0x3d, 0xe4, 0x25, 0x3c, 0x18, 0xf7, 0xd3, 0xb2, 0xaf},
	{0x6e, 0x3d, 0x69, 0x52, 0x90, 0xde, 0xf5, 0x17, 0x85, 0x7c, 0x8e, 0x74, 0x3d, 0xc6, 0x51, 0x61, 0x47, 0x9f, 0x0c, 0x04},
	{0x7b, 0xcf, 0x45, 0xf0, 0xb7, 0xbd, 0xa7, 0x03, 0xeb, 0x76, 0x2b, 0x2a, 0x7a, 0xb3, 0x22, 0x9d, 0xad, 0x2a, 0x14, 0x20},
	{0xd1, 0x75, 0xf2, 0x42, 0x79, 0x0f, 0x94, 0x0a, 0x74, 0xe1, 0xe7, 0xa9, 0x85, 0x49, 0xfa, 0x42, 0x05, 0x87, 0x96, 0xa7},
	{0x3e, 0x66, 0x8e, 0x02, 0x48, 0x0a, 0x1f, 0xe8, 0xd1, 0xdc, 0x05, 0x1e, 0x33, 0x13, 0xd1, 0x2f, 0xa4, 0xe0, 0xbe, 0xfc},
	{0x82, 0xf5, 0x8d, 0x66, 0x97, 0xcc, 0x9e, 0x87, 0xb6, 0x66, 0xc6, 0xcc, 0xd1, 0x20, 0xcb, 0xdd, 0x66, 0xad, 0xea, 0x98},
	{0x96, 0x2b, 0xd7, 0x5a, 0x5c, 0x86, 0xa5, 0x88, 0x95, 0x66, 0x6e, 0xc4, 0x47, 0xbd, 0x0a, 0x6c, 0x71, 0xf1, 0x77, 0x68},
	{0x69, 0x90, 0xaf, 0x73, 0x0d, 0x7c, 0xb7, 0x01, 0xe8, 0xb0, 0x09, 0xed, 0x7a, 0x70, 0x91, 0xfc, 0xa1, 0x6c, 0xce, 0x67},
	{0x50, 0x39, 0x92, 0x85, 0xa7, 0xe0, 0xaf, 0x39, 0xea, 0xb8, 0x20, 0x8f, 0x9e, 0x24, 0x2b, 0x05, 0x31, 0x72, 0x65, 0x86},
	{0x42, 0xc4, 0xc5, 0x5a, 0xeb, 0x0e, 0x16, 0xfa, 0x10, 0x55, 0xe0, 0x0e, 0xf5, 0x9e, 0x46, 0xc6, 0x8c, 0x02, 0xa2, 0x88},
	{0x85, 0x4b, 0x43, 0x75, 0x9a, 0x1e, 0x8b, 0x31, 0x36, 0x85, 0xcd, 0x7b, 0x14, 0x2a, 0x39, 0xb1, 0xe4, 0xce, 0x14, 0xdf},
	{0xeb, 0x29, 0xcf, 0xec, 0x83, 0x06, 0x9b, 0x05, 0xfd, 0x2c, 0xdb, 0x37, 0x52, 0x66, 0xbd, 0xf6, 0xc4, 0x8d, 0x95, 0xab},
	{0x9b, 0xc7, 0xdd, 0x71, 0x09, 0x3d, 0x92, 0x29, 0x3c, 0x22, 0x82, 0x9b, 0x2b, 0x4a, 0x78, 0xfc, 0x2c, 0xb8, 0x13, 0xda},
	{0x6a, 0x30, 0xc1, 0xb5, 0xf4, 0x43, 0xe6, 0x0b, 0xee, 0xc3, 0xc4, 0x1c, 0x28, 0xd5, 0x5c, 0xe3, 0xc7, 0xe0, 0x6d, 0x97},
	{0xd5, 0xa5, 0xf8, 0xfd, 0xce, 0x71, 0x15, 0xb9, 0xf4, 0xe6, 0xd7, 0x43, 0x0a, 0xfa, 0x65, 0x52, 0xc6, 0xba, 0x01, 0x47},
	{0x29, 0x90, 0x5c, 0x92, 0x4d, 0xc5, 0xfd, 0xe3, 0x9e, 0xf3, 0x30, 0x63, 0xc3, 0xb9, 0xae, 0x1d, 0x13, 0x6e, 0xd4, 0x2b},
	{0xef, 0x1c, 0xb3, 0xe8, 0x7c, 0x46, 0x33, 0xf0, 0xce, 0xda, 0xa8, 0xe4, 0xd8, 0x11, 0xaf, 0xf7, 0xd0, 0xa9, 0x85, 0x3a},
	{0x62, 0x17, 0x0f, 0xa6, 0xed, 0x4f, 0x02, 0xdb, 0x28, 0xff, 0x92, 0xdf, 0xae, 0xe3, 0xb8, 0x83, 0x64, 0x28, 0x44, 0xa6},
	{0x77, 0x57, 0x7c, 0xa3, 0x49, 0x53, 0x2e, 0xa4, 0x44, 0x32, 0x19, 0x66, 0x70, 0xc9, 0x73, 0xa2, 0xdd, 0x40, 0x6b, 0x64},
	{0x3e, 0xe8, 0xce, 0x47, 0x9d, 0xb3, 0xe0, 0x98, 0x8f, 0xae, 0x1a, 0xd8, 0xfa, 0x0f, 0x1f, 0xa1, 0x7c, 0x3f, 0xb7, 0xa2},
	{0xbd, 0x35, 0x8e, 0xad, 0x30, 0xa7, 0xd2, 0xcb, 0x23, 0xfe, 0x35, 0xaa, 0xa8, 0x2a, 0x5c, 0x8c, 0xec, 0x82, 0xdb, 0x0d},
	{0x40, 0x50, 0xd9, 0x48, 0x59, 0xbf, 0x5f, 0x5e, 0x50, 0x1c, 0xc3, 0x15, 0x36, 0xad, 0xd1, 0x96, 0xb2, 0xaf, 0xa2, 0xfd},
	{0xfe, 0x0a, 0x0f, 0x5a, 0x0d, 0x7f, 0xac, 0xb8, 0x1a, 0x2d, 0x11, 0xb6, 0xd2, 0xb6, 0x50, 0x4d, 0xb1, 0x0e, 0xd9, 0xf8},
	{0xcb, 0xa3, 0x00, 0xe6, 0xc5, 0x77, 0xd1, 0x7c, 0xde, 0x98, 0x61, 0x76, 0xc8, 0x40, 0xe1, 0xb3, 0x3f, 0x20, 0x03, 0x70},
	{0x74, 0x4f, 0x9a, 0xe0, 0xd2, 0xf7, 0xfa, 0x18, 0xb4, 0xa0, 0x7e, 0x0e, 0x78, 0x86, 0x41, 0x66, 0x0f, 0x31, 0xa6, 0x52},
	{0x26, 0xcb, 0x19, 0xea, 0x23, 0x76, 0x77, 0xee, 0x80, 0x94, 0x84, 0x3e, 0x28, 0x19, 0xc2, 0x13, 0x65, 0x3b, 0xf6, 0x37},
	{0x40, 0xff, 0x40, 0x9f, 0x5f, 0x15, 0x67, 0xef, 0xff, 0x00, 0x66, 0x8e, 0x64, 0x81, 0x1b, 0xe9, 0xba, 0x10, 0x09, 0x53},
	{0x14, 0x88, 0xbe, 0xb6, 0x89, 0xb5, 0xec, 0x15, 0x3a, 0x68, 0x28, 0x0c, 0x28, 0x41, 0xc0, 0x05, 0x4b, 0x44, 0xd3, 0xf4},
	{0xbe, 0xcc, 0xfa, 0x3a, 0xf4, 0x6f, 0x08, 0x70, 0x4d, 0xf1, 0x1c, 0x1b, 0xa7, 0x05, 0x64, 0xff, 0x8b, 0x3e, 0xc4, 0x63},
	{0x89, 0xb4, 0xa4, 0xf2, 0x89, 0xd9, 0x0b, 0x5d, 0x10, 0xad, 0x94, 0x72, 0xdf, 0x69, 0x39, 0x51, 0xe3, 0xd8, 0xf7, 0xbc},
	{0x53, 0x4f, 0x9c, 0x0c, 0xdb, 0x6c, 0x87, 0x42, 0x1b, 0xb4, 0x4d, 0x18, 0xde, 0xda, 0xe9, 0xa1, 0xf5, 0xc0, 0x60, 0xd7},
	{0x12, 0x80, 0x02, 0x07, 0x6e, 0x4a, 0xc3, 0xc7, 0x56, 0x97, 0xfb, 0x4e, 0xfd, 0xf1, 0xf8, 0xdd, 0xcc, 0x97, 0x1f, 0xbe},
	{0xd6, 0x44, 0x5a, 0xc1, 0x39, 0xb1, 0xba, 0x6c, 0xfb, 0x92, 0x97, 0xac, 0x4f, 0x00, 0x14, 0x98, 0xac, 0x10, 0x54, 0x51},
	{0x6c, 0xfc, 0x64, 0x25, 0x93, 0x79, 0x82, 0x17, 0x45, 0x6f, 0x0a, 0x54, 0xd2, 0x24, 0x01, 0xca, 0x09, 0x30, 0x50, 0x24},
	{0x1f, 0x1b, 0x37, 0x22, 0x55, 0xc0, 0xea, 0xca, 0xa3, 0xd5, 0x38, 0x1d, 0xd7, 0x7f, 0x62, 0x03, 0x0c, 0xe6, 0xf1, 0x7e},
	{0xab, 0x87, 0x4e, 0xbc, 0x9c, 0xbb, 0xab, 0x54, 0xa5, 0xeb, 0x64, 0x66, 0xe2, 0x48, 0xeb, 0xa5, 0x08, 0x56, 0xca, 0xa4},
	{0x9c, 0xc1, 0x4e, 0xf3, 0xb3, 0xe8, 0xf0, 0x39, 0x2b, 0xbd, 0x05, 0x03, 0x71, 0x8b, 0xb4, 0x32, 0x3a, 0x53, 0x55, 0x94},
	{0xac, 0x8c, 0x38, 0x65, 0xa1, 0x43, 0xb2, 0xe0, 0x38, 0x69, 0xf1, 0x5a, 0x5b, 0x86, 0xe5, 0x60, 0xf6, 0x0a, 0xd6, 0x32},
	{0x30, 0x69, 0xeb, 0xa0, 0x3e, 0xf7, 0xdc, 0x5d, 0x48, 0xf9, 0x5c, 0x5f, 0xa0, 0x14, 0x17, 0xc8, 0x45, 0xfe, 0xe3, 0xa3},
	{0x9d, 0x82, 0x7a, 0x9f, 0xb2, 0x07, 0x4a, 0x62, 0xa3, 0xcb, 0x00, 0x6c, 0x66, 0x4c, 0xf6, 0xae, 0xdf, 0x53, 0x1b, 0x4a},
	{0x7d, 0x80, 0x7a, 0x1f, 0x4e, 0x2a, 0xff, 0x6f, 0x8d, 0xe5, 0x53, 0xce, 0xc8, 0x1e, 0xfb, 0x03, 0x69, 0x9f, 0x65, 0x31},
	{0x34, 0x0b, 0x4b, 0x3d, 0xbd, 0xa0, 0xe0, 0x2a, 0x1e, 0xd8, 0x0c, 0x77, 0xf2, 0x48, 0x40, 0xdf, 0x99, 0x5d, 0x6a, 0x81},
	{0xca, 0x25, 0x92, 0x6e, 0x81, 0xa5, 0x29, 0x41, 0xd8, 0x82, 0xc9, 0xa9, 0x69, 0x6d, 0xcc, 0x9b, 0xa0, 0x08, 0x7e, 0x35},
	{0x9f, 0xce, 0x33, 0xbd, 0x23, 0x39, 0xd6, 0x7f, 0x1c, 0x43, 0x5c, 0xaa, 0xac, 0xaf, 0x1e, 0x3e, 0x0e, 0x8a, 0x70, 0xd5},
	{0xfa, 0xea, 0x33, 0xbe, 0x37, 0x8d, 0x82, 0xfa, 0x11, 0x04, 0x5f, 0x0a, 0x33, 0x51, 0xb6, 0x90, 0x0c, 0x05, 0x7a, 0x3a},
	{0xe0, 0x79, 0x4b, 0x0c, 0x2a, 0xa1, 0xf7, 0x7f, 0xa6, 0x2f, 0x69, 0x2a, 0x90, 0xdf, 0x71, 0x29, 0x99, 0x81, 0xe1, 0x75},
	{0x89, 0x53, 0x42, 0xe1, 0x33, 0x6d, 0xc1, 0xf5, 0xce, 0x63, 0xf9, 0xa7, 0xc5, 0xc6, 0xa4, 0xde, 0x8f, 0x81, 0x7a, 0x6e},
	{0x64, 0x58, 0xbf, 0x25, 0xda, 0x49, 0x06, 0x47, 0x9a, 0x01, 0xff, 0xdc, 0xaa, 0x6d, 0x46, 0x6e, 0x22, 0x72, 0x2e, 0x01},
	{0x59, 0x36, 0xe3, 0xf1, 0xec, 0x8d, 0xc1, 0xa0, 0x13, 0x3d, 0xbc, 0x35, 0x24, 0xb1, 0xf2, 0x1e, 0x73, 0x5f, 0xc6, 0x74},
	{0x4d, 0x75, 0x38, 0x8a, 0x13, 0x6f, 0x7a, 0xb7, 0x6d, 0x8f, 0x8e, 0x0b, 0xda, 0xc8, 0x84, 0xdb, 0x7d, 0xd7, 0xd5, 0x9e},
	{0x6a, 0x56, 0x0b, 0x60, 0x6a, 0x3e, 0x63, 0xe7, 0xdf, 0xa4, 0x33, 0x5c, 0x06, 0x43, 0x36, 0xe2, 0x57, 0x20, 0xeb, 0xb8},
	{0x7f, 0xb3, 0xb5, 0x6f, 0x69, 0x6d, 0xe9, 0x37, 0xf0, 0x5c, 0xdd, 0x4f, 0x06, 0x5c, 0xab, 0xc8, 0x2d, 0xfe, 0xf2, 0xab},
	{0xb0, 0x14, 0xe3, 0x74, 0xf1, 0xdb, 0x58, 0x35, 0x89, 0xec, 0xe2, 0x3f, 0x91, 0x61, 0x23, 0x61, 0x61, 0xf8, 0xf7, 0x8c},
	{0x6c, 0x60, 0x4b, 0xf6, 0x13, 0xb2, 0x58, 0x3b, 0x1d, 0xca, 0x1e, 0xd6, 0x94, 0x05, 0x6b, 0xbe, 0xed, 0x26, 0x1c, 0x07},
	{0x8f, 0x5e, 0xb7, 0x03, 0x05, 0xff, 0x26, 0x6e, 0x8e, 0x4d, 0xa9, 0x4b, 0xf4, 0x3c, 0x43, 0xf8, 0x85, 0x2f, 0x83, 0x76},
	{0x74, 0xda, 0x77, 0x48, 0x7f, 0x9f, 0xf6, 0xfa, 0xf8, 0xe3, 0xc2, 0xf2, 0xdd, 0x6f, 0x1d, 0x60, 0x7c, 0xb7, 0x3e, 0x77},
	{0xb8, 0x96, 0x8b, 0x1f, 0xfc, 0xca, 0x87, 0x58, 0xc7, 0xa2, 0x60, 0x75, 0xd8, 0x79, 0x56, 0xf1, 0x7c, 0x8d, 0x11, 0xf7},
	{0x6f, 0xdc, 0x02, 0x7a, 0xdc, 0xba, 0x80, 0x84, 0x7b, 0x2a, 0x4e, 0xc3, 0x4f, 0xd3, 0x16, 0x0b, 0x3d, 0x57, 0x4e, 0x4d},
	{0x0f, 0x29, 0xbe, 0x80, 0xbf, 0x2d, 0xf0, 0xe9, 0x7b, 0x1b, 0x5e, 0x08, 0xe2, 0x66, 0xc8, 0x62, 0x2a, 0xa2, 0xa7, 0xdf},
	{0x5d, 0x88, 0xe2, 0xd3, 0x32, 0x7a, 0x93, 0x47, 0xd4, 0x2a, 0x53, 0x9a, 0x85, 0x7c, 0x3e, 0xe9, 0x45, 0x5c, 0x5b, 0xc4},
	{0xfa, 0x0c, 0xae, 0x0a, 0x9f, 0x00, 0x54, 0xf7, 0xbb, 0xf0, 0x8b, 0x10, 0x81, 0x63, 0xfc, 0x19, 0x20, 0x97, 0x84, 0xad},
	{0xc4, 0x65, 0xde, 0x26, 0x32, 0xaf, 0xf5, 0xf1, 0x3c, 0xc6, 0xe2, 0x03, 0x73, 0xbe, 0xd2, 0x74, 0x64, 0xb0, 0x69, 0xd5},
	{0x85, 0x2b, 0xa0, 0x99, 0xd5, 0x2e, 0x9c, 0x26, 0x7f, 0xc7, 0xce, 0x78, 0xe5, 0x4c, 0x3e, 0x6e, 0x3f, 0x8c, 0x7a, 0x7d},
	{0xa5, 0x64, 0x05, 0xde, 0x5b, 0x4c, 0xe5, 0x94, 0xc7, 0x9a, 0xf0, 0xf2, 0x09, 0xeb, 0xb0, 0xbf, 0x66, 0x2c, 0x3c, 0x5c},
	{0xfa, 0x5c, 0x38, 0xd3, 0x29, 0x65, 0x4d, 0x10, 0xf5, 0x94, 0x8e, 0x29, 0x7e, 0xec, 0x7e, 0x54, 0x9b, 0x75, 0x70, 0x27},
	{0x37, 0x3d, 0x72, 0x91, 0x5a, 0xa4, 0x3d, 0xd0, 0xef, 0x46, 0xed, 0xbb, 0x61, 0xe1, 0xa5, 0xcd, 0x4a, 0x3b, 0x87, 0x27},
	{0x2a, 0xe5, 0x66, 0x08, 0xcf, 0x2b, 0x91, 0x08, 0xc7, 0x42, 0xcd, 0x14, 0x14, 0x53, 0x82, 0xf3, 0xef, 0x86, 0x10, 0xbd},
	{0xc7, 0xfd, 0xdf, 0x8a, 0xdf, 0x95, 0x1b, 0xdc, 0xd1, 0xd4, 0x2c, 0xa2, 0x1a, 0xc4, 0xdf, 0x88, 0x51, 0x1c, 0x1c, 0x8c},
	{0x66, 0x1c, 0xf4, 0x57, 0x5f, 0x0e, 0x87, 0x3b, 0xd0, 0xd2, 0x32, 0xa5, 0x6f, 0x6d, 0x69, 0x3f, 0xb4, 0x16, 0xa7, 0x0a},
	{0xd0, 0x68, 0x34, 0xee, 0x4e, 0xf9, 0xce, 0x77, 0x8c, 0x1a, 0xb7, 0x3f, 0x57, 0x6c, 0x5d, 0x8f, 0xe2, 0xba, 0x47, 0xb8},
	{0x51, 0xb5, 0x09, 0x7f, 0x95, 0x78, 0x09, 0xd1, 0xd2, 0x50, 0x7f, 0x41, 0xa4, 0xc7, 0x9e, 0xfc, 0x1f, 0x4f, 0xbe, 0xaf},
	{0xcd, 0xcb, 0xa6, 0x7f, 0x17, 0x99, 0x30, 0x9a, 0x42, 0x64, 0x44, 0x14, 0x4c, 0x73, 0x0d, 0x63, 0xe6, 0x7a, 0x0b, 0x8b},
	{0x79, 0x7d, 0xa5, 0xfa, 0xc8, 0x2f, 0x16, 0x9b, 0xb8, 0x49, 0xb7, 0x99, 0xd2, 0x91, 0x71, 0x6f, 0x24, 0x68, 0xd1, 0x5b},
	{0x48, 0x3d, 0xed, 0xfa, 0xf3, 0x67, 0x59, 0x49, 0x73, 0xd8, 0x10, 0x98, 0x6d, 0xa6, 0x4f, 0xcd, 0x20, 0x40, 0x5b, 0x14},
	{0x02, 0x42, 0x83, 0x01, 0xf3, 0x50, 0xbc, 0xba, 0xb6, 0x89, 0xad, 0x29, 0x13, 0x70, 0xd3, 0x93, 0xeb, 0xb3, 0x9a, 0xba},
	{0x4b, 0x74, 0xe8, 0x94, 0x6c, 0x1b, 0xf1, 0x3e, 0xeb, 0x48, 0x8c, 0x5e, 0x6f, 0x66, 0x8d, 0xda, 0xb3, 0xc1, 0x97, 0xd0},
	{0xb0, 0xc1, 0x1d, 0x46, 0x9e, 0x92, 0xaf, 0x6a, 0x27, 0x47, 0x2e, 0x24, 0x59, 0x44, 0xda, 0x48, 0x62, 0x99, 0xb1, 0x7c},
	{0xaa, 0x61, 0x99, 0x65, 0xd3, 0xb9, 0x2b, 0x9c, 0x94, 0x60, 0x24, 0x42, 0x67, 0x29, 0x53, 0xfc, 0x3a, 0x83, 0xc6, 0x20},
	{0x04, 0xfe, 0xf9, 0xc9, 0x77, 0x27, 0xea, 0xac, 0xfa, 0xaa, 0x1c, 0xaa, 0xcd, 0xf2, 0x1d, 0xec, 0x3a, 0xf6, 0x1d, 0x46},
	{0x64, 0x7e, 0x33, 0x89, 0x3c, 0xe9, 0xe5, 0x75, 0x70, 0xf9, 0x3b, 0x8f, 0xbd, 0xf7, 0x2f, 0xd9, 0xec, 0x71, 0x78, 0x61},
	{0xac, 0xb0, 0x7c, 0x14, 0x09, 0x3d, 0xdc, 0xee, 0x56, 0x30, 0x55, 0x29, 0x2f, 0xe1, 0xbe, 0x6b, 0xe4, 0x2a, 0xea, 0xc2},
	{0xa0, 0x5e, 0x93, 0x40, 0xb6, 0x8f, 0xae, 0x50, 0xa1, 0xe0, 0x25, 0x9c, 0x13, 0x6e, 0x77, 0xb9, 0x6e, 0x7a, 0x53, 0x9d},
	{0x05, 0x38, 0xd8, 0x40, 0x0a, 0xaf, 0x1f, 0x2d, 0x44, 0xf6, 0x7a, 0xb7, 0x4a, 0x94, 0x73, 0x80, 0x6f, 0x15, 0x41, 0xfe},
	{0x8d, 0xb6, 0x71, 0x2e, 0x0b, 0x38, 0x4b, 0xbe, 0x41, 0x77, 0xd1, 0xd6, 0x3b, 0xf7, 0xe2, 0xf7, 0x92, 0x9f, 0x43, 0x2b},
	{0xee, 0x31, 0x41, 0x13, 0x01, 0x69, 0x53, 0xbc, 0x3a, 0x60, 0x55, 0x8b, 0x58, 0xac, 0xa2, 0x26, 0xdb, 0xcc, 0xb4, 0x80},
	{0xf1, 0x21, 0x9a, 0x29, 0x3a, 0xc8, 0x06, 0x3d, 0x81, 0x75, 0x71, 0xcd, 0x74, 0xe3, 0x4e, 0xdf, 0x7e, 0x46, 0xb5, 0x1f},
	{0xa0, 0xc1, 0x88, 0xb9, 0xbe, 0x3e, 0x3b, 0x54, 0x48, 0x9e, 0x02, 0x76, 0x10, 0x12, 0xbd, 0x5c, 0xa8, 0xea, 0x51, 0xea},
	{0xcb, 0x8d, 0xc0, 0xa1, 0x7d, 0x4f, 0x48, 0xcd, 0x33, 0x8c, 0x7f, 0xf5, 0x7a, 0xc9, 0x6c, 0x77, 0x47, 0x98, 0xbe, 0x0b},
	{0x45, 0x12, 0xc8, 0x14, 0x9e, 0x63, 0xe5, 0xed, 0x15, 0xf4, 0x50, 0x05, 0xd7, 0xfb, 0x5b, 0xe0, 0x04, 0x1f, 0x66, 0xf6},
	{0xd3, 0x85, 0x24, 0x71, 0xd1, 0xd6, 0xf3, 0xa4, 0x3a, 0xdf, 0x9b, 0xa0, 0x78, 0x31, 0xe9, 0x2a, 0xd5, 0x09, 0xdb, 0xd7},
	{0x97, 0x23, 0x32, 0xa7, 0x4e, 0xa0, 0xf6, 0x25, 0xd3, 0x8e, 0x2a, 0xbc, 0x28, 0xb4, 0xf9, 0xf7, 0xd3, 0x9b, 0xba, 0x08},
	{0x76, 0x44, 0x96, 0xa4, 0x62, 0xfd, 0xb7, 0xdb, 0x6b, 0x89, 0xc4, 0x91, 0x95, 0xf5, 0xae, 0x26, 0xa9, 0x95, 0x7c, 0xbb},
	{0x52, 0xad, 0xf1, 0xda, 0x63, 0xd1, 0x3a, 0x4f, 0x5c, 0x85, 0xde, 0xe1, 0x78, 0x06, 0xb2, 0xd8, 0xbf, 0x23, 0x06, 0x17},
	{0x29, 0xf2, 0xdf, 0xe6, 0x5c, 0xd9, 0xea, 0xc8, 0x7e, 0xaa, 0xb1, 0x32, 0x23, 0xc4, 0x30, 0x1e, 0x2c, 0xd6, 0xb4, 0xdf},
	{0x83, 0x56, 0xbb, 0x2b, 0x3e, 0x50, 0x3b, 0x54, 0x0a, 0xcd, 0xa3, 0xa3, 0x71, 0xbe, 0xb8, 0x63, 0x17, 0x38, 0xc6, 0xce},
	{0x9a, 0x80, 0x41, 0xaf, 0x6d, 0x1a, 0xe1, 0x63, 0xbc, 0x44, 0xe6, 0x3f, 0x95, 0x82, 0x51, 0x6f, 0x76, 0x6f, 0xdb, 0x94},
	{0xbc, 0x90, 0x24, 0x5b, 0x0e, 0x5e, 0xfc, 0xec, 0x3f, 0xe6, 0xe5, 0x8f, 0x16, 0xc0, 0x37, 0x67, 0xa3, 0x0c, 0x74, 0xc6},
	{0x95, 0xaf, 0xe6, 0xf6, 0xd1, 0xd0, 0x90, 0x28, 0xc8, 0xec, 0x1d, 0xc3, 0x3d, 0xa2, 0xf8, 0xf0, 0xd7, 0xfa, 0x8c, 0x58},
	{0x36, 0xdf, 0x33, 0x8e, 0x8a, 0x60, 0x16, 0xde, 0xc0, 0xc5, 0xa4, 0x35, 0xe3, 0xf3, 0x25, 0x6a, 0x8c, 0x8a, 0xe8, 0xe5},
	{0xce, 0x2d, 0x63, 0x5f, 0xe8, 0xc4, 0x4e, 0x6f, 0xbb, 0x84, 0xf9, 0x7f, 0x28, 0x52, 0x75, 0xb4, 0xaa, 0x35, 0xd4, 0xde},
	{0x24, 0x0d, 0xc3, 0xe0, 0xdc, 0x2d, 0x65, 0xe9, 0x30, 0x7b, 0x19, 0xb4, 0x49, 0xcd, 0xdb, 0xe8, 0x42, 0x83, 0xf4, 0x57},
	{0xa3, 0x92, 0x9c, 0x58, 0x9d, 0x34, 0xd1, 0x92, 0x1c, 0xaa, 0x50, 0xdc, 0xaf, 0xac, 0x80, 0x78, 0x04, 0x7c, 0x67, 0xa7},
	{0x98, 0x19, 0x91, 0x9f, 0x04, 0xd9, 0xe0, 0x25, 0xce, 0x92, 0xe1, 0x74, 0x30, 0x26, 0x2d, 0x01, 0xd1, 0x45, 0x70, 0xb9},
	{0x9b, 0xb0, 0x63, 0x88, 0xa0, 0x3a, 0xd9, 0xb7, 0x20, 0x39, 0xc0, 0x22, 0x73, 0x26, 0x1b, 0x3f, 0x68, 0x78, 0x55, 0xae},
	{0x95, 0x2d, 0x30, 0x16, 0x0e, 0x92, 0xa7, 0xbb, 0x7d, 0xb0, 0x39, 0x9b, 0x75, 0xc7, 0xf5, 0xc8, 0xe8, 0xc2, 0x1f, 0xb4},
	{0x9b, 0x14, 0xd5, 0xa4, 0x93, 0x60, 0xa7, 0x20, 0x2b, 0x9f, 0x1d, 0xc1, 0x4d, 0x13, 0xce, 0x9b, 0xe2, 0x64, 0x06, 0x74},
	{0x81, 0xa7, 0xec, 0x40, 0xe8, 0x7e, 0x52, 0x28, 0xe1, 0x79, 0x88, 0x85, 0xb3, 0x3d, 0x53, 0x07, 0xec, 0x59, 0xe1, 0x6e},
	{0x04, 0x3e, 0x8f, 0x20, 0xf2, 0xf3, 0x75, 0x79, 0x6d, 0xcc, 0xdf, 0xf4, 0x41, 0x27, 0xc0, 0x3b, 0x5d, 0x67, 0xf2, 0xca},
	{0x00, 0x52, 0xf3, 0xb5, 0x49, 0xa0, 0xed, 0xbb, 0x5a, 0xc7, 0x0c, 0x1c, 0xab, 0x0d, 0x8c, 0x1f, 0xcb, 0xde, 0x61, 0x8b},
	{0x64, 0x0f, 0xc9, 0xa7, 0xad, 0x3b, 0xff, 0x48, 0x80, 0x1a, 0x37, 0x88, 0x98, 0x25, 0xd6, 0x88, 0xb8, 0xb7, 0x0a, 0xce},
	{0x89, 0xdd, 0x0d, 0xe5, 0x98, 0xc8, 0x3e, 0xb9, 0x71, 0x4f, 0x68, 0x39, 0x49, 0x9f, 0x32, 0x2d, 0xfc, 0xe2, 0xb6, 0x93},
	{0xa6, 0x45, 0x0a, 0x2f, 0x52, 0xd9, 0x32, 0xaf, 0xb8, 0x21, 0x47, 0xab, 0x94, 0xfb, 0x45, 0xfb, 0x4e, 0xaa, 0x32, 0xac},
	{0x19, 0x98, 0x78, 0xc9, 0x03, 0xf7, 0x62, 0xbb, 0x98, 0x9d, 0x18, 0x71, 0x43, 0xa2, 0xb4, 0x0d, 0xb9, 0xa2, 0xaa, 0x3e},
	{0x65, 0xaa, 0x13, 0x29, 0x33, 0x57, 0x52, 0x0a, 0xf8, 0x47, 0xb9, 0xfc, 0xcd, 0xff, 0x79, 0x0c, 0x42, 0x91, 0x67, 0x9d},
	{0x7f, 0x25, 0x95, 0x2c, 0x37, 0x18, 0x53, 0xf9, 0x7b, 0x33, 0x26, 0x56, 0x04, 0xc4, 0xd0, 0xcc, 0x1d, 0x48, 0xea, 0xba},
	{0x71, 0x98, 0x6b, 0x2a, 0x33, 0x13, 0x9d, 0x65, 0x35, 0x87, 0x7d, 0x37, 0xfd, 0x78, 0xd2, 0xe4, 0xaf, 0x4b, 0xf9, 0x96},
	{0xbb, 0x52, 0x61, 0x34, 0xb2, 0x8e, 0x97, 0x1b, 0x71, 0x8b, 0xcf, 0xb9, 0x10, 0x85, 0x8b, 0x00, 0xc9, 0x30, 0x5b, 0x6b},
	{0x4e, 0x97, 0xb4, 0x5a, 0x38, 0xbd, 0xfc, 0xe2, 0xa1, 0x09, 0x5e, 0x34, 0xd8, 0xbc, 0xd5, 0xca, 0x92, 0xa0, 0x40, 0x1a},
	{0x8d, 0xaf, 0xa8, 0xdb, 0x9c, 0x92, 0xf0, 0xb2, 0x0b, 0xde, 0xfe, 0x0a, 0x62, 0x35, 0xe7, 0x9f, 0x01, 0x10, 0x93, 0xc0},
	{0x2a, 0xea, 0x03, 0x7b, 0x7c, 0x95, 0x7a, 0xb2, 0x2b, 0x2f, 0x06, 0x38, 0x81, 0xec, 0x9a, 0x8c, 0x4f, 0x23, 0x61, 0xb8},
	{0xcd, 0xf4, 0xe9, 0x01, 0x07, 0x8c, 0x4b, 0x16, 0x23, 0x2d, 0x8c, 0x6c, 0x11, 0x85, 0x23, 0x38, 0x74, 0x87, 0x90, 0x0f},
	{0xaa, 0xc3, 0x91, 0xf2, 0x7e, 0x02, 0x69, 0xd8, 0x91, 0xc1, 0xc1, 0x85, 0xeb, 0x57, 0xb4, 0x67, 0xc1, 0xc6, 0x87, 0x0b},
	{0xeb, 0x4e, 0x8d, 0x70, 0xd1, 0x6e, 0xe8, 0x57, 0x63, 0x7a, 0xb8, 0x97, 0x33, 0x48, 0xce, 0x31, 0x5b, 0x84, 0xc2, 0x56},
	{0xd0, 0x19, 0x26, 0x6d, 0x39, 0xbc, 0x12, 0x8a, 0x60, 0x8b, 0x49, 0xb5, 0x36, 0x61, 0x30, 0xd9, 0x9a, 0x00, 0xfd, 0x92},
	{0x83, 0x06, 0x96, 0xec, 0x42, 0x7f, 0x05, 0x79, 0xeb, 0xeb, 0x5d, 0xd5, 0x59, 0x25, 0x87, 0x2f, 0xe3, 0xae, 0x78, 0x76},
	{0x12, 0xf8, 0x8e, 0xec, 0x0e, 0x1d, 0xb9, 0x9f, 0xd9, 0x6f, 0x6c, 0x50, 0xe4, 0xa6, 0x9d, 0x29, 0xd1, 0xba, 0xb3, 0xd7},
	{0x21, 0x9d, 0x64, 0x9b, 0xbd, 0x03, 0xee, 0xd7, 0x94, 0x07, 0x31, 0x46, 0x61, 0x5a, 0xef, 0xa8, 0xdb, 0x78, 0xd4, 0xc2},
	{0x4f, 0x33, 0xcc, 0x27, 0xdc, 0xc9, 0x48, 0xa6, 0xdc, 0x88, 0x54, 0x2d, 0xbf, 0xe0, 0x5a, 0xfc, 0xf4, 0xf4, 0xcd, 0x63},
	{0xd4, 0x0a, 0x9a, 0x72, 0x2b, 0x78, 0xf5, 0x4f, 0xaa, 0xf4, 0x90, 0x7c, 0x42, 0x58, 0x6a, 0xe4, 0x54, 0xd6, 0x9d, 0xd3},
	{0x83, 0xd6, 0x07, 0x2d, 0xc0, 0x7b, 0x3d, 0x06, 0xc0, 0x59, 0x06, 0xd3, 0x91, 0xe6, 0x92, 0x39, 0x60, 0x0e, 0xc3, 0xbd},
	{0x0e, 0xff, 0x7c, 0x14, 0xa0, 0x04, 0x33, 0x82, 0x36, 0x49, 0x3e, 0x71, 0x79, 0x4c, 0x78, 0x66, 0x1a, 0x12, 0x64, 0xe2},
	{0x95, 0xed, 0x2a, 0xef, 0x0a, 0xa2, 0x06, 0xb3, 0x14, 0xa8, 0x21, 0xe4, 0xc2, 0x16, 0x88, 0xf6, 0xbd, 0x7a, 0xf1, 0x26},
	{0x30, 0xf7, 0x66, 0xf7, 0x5a, 0x09, 0x8d, 0xbc, 0xb3, 0xa5, 0x24, 0xde, 0x1c, 0x47, 0xbb, 0x76, 0xfe, 0x11, 0x96, 0xde},
	{0x8b, 0x10, 0x8c, 0x55, 0x9f, 0x05, 0xe0, 0x22, 0x1b, 0x6b, 0x3e, 0x12, 0x1e, 0x35, 0x73, 0x81, 0x44, 0xb0, 0x74, 0x99},
	{0xc2, 0x07, 0x89, 0xf7, 0x66, 0x8c, 0x2d, 0xa2, 0xb7, 0x9c, 0x90, 0xd1, 0xa0, 0x41, 0x36, 0x26, 0x9c, 0xbb, 0x16, 0xfe},
	{0x6c, 0x41, 0xa6, 0xad, 0x41, 0x2f, 0x48, 0xf8, 0x1a, 0x9d, 0x5e, 0xdf, 0x59, 0xdc, 0xde, 0xcc, 0x35, 0x83, 0x98, 0xbf},
	{0xd9, 0x49, 0xa8, 0x54, 0x3d, 0x24, 0x64, 0xd9, 0x55, 0xf5, 0xdc, 0x4b, 0x07, 0x77, 0xca, 0xc8, 0x63, 0xf4, 0x87, 0x29},
	{0x80, 0x1c, 0xd4, 0x4b, 0x62, 0x77, 0xfa, 0xe0, 0x79, 0x2a, 0xa7, 0x7f, 0x24, 0xd7, 0x84, 0xf4, 0x7d, 0x92, 0x24, 0x18},
	{0xfc, 0x5c, 0x02, 0x06, 0xfe, 0x39, 0x2a, 0x0d, 0xda, 0xd4, 0xdc, 0x93, 0x63, 0xfd, 0xe2, 0xd3, 0xe3, 0xd1, 0xe6, 0x81},
	{0xc6, 0xc5, 0x66, 0x49, 0xcf, 0x68, 0xbb, 0x80, 0xe9, 0x5a, 0xb8, 0x79, 0xa6, 0x9c, 0xa2, 0xaa, 0x26, 0xb9, 0xd2, 0x28},
	{0x4f, 0x63, 0x32, 0x98, 0x0d, 0xd6, 0x79, 0x5d, 0xb8, 0x5a, 0xbe, 0x90, 0xd8, 0x78, 0x4b, 0x9a, 0x92, 0xb0, 0x76, 0xfa},
	{0xd7, 0xcb, 0x57, 0xa5, 0x82, 0xa5, 0x45, 0xd5, 0x26, 0x12, 0x51, 0x68, 0x47, 0x04, 0x03, 0xe0, 0x57, 0x01, 0xc9, 0x62},
	{0xf6, 0x9a, 0x78, 0xf2, 0xab, 0xf4, 0xbd, 0xaf, 0x3e, 0x95, 0xa1, 0x76, 0x23, 0xc6, 0xf3, 0x32, 0xec, 0x6e, 0xba, 0xb8},
	{0x37, 0xfd, 0x0d, 0x8f, 0x71, 0xad, 0xbb, 0xf7, 0x2c, 0xa9, 0xf0, 0x06, 0xb9, 0xa5, 0xcf, 0x54, 0xbf, 0x00, 0x70, 0x2d},
	{0x02, 0xdf, 0x3b, 0xbd, 0x96, 0x1e, 0xeb, 0xfc, 0xe2, 0x04, 0xd7, 0xa8, 0xd8, 0xae, 0x9e, 0xc4, 0x06, 0x00, 0xa8, 0xaa},
	{0x6a, 0x32, 0xae, 0x29, 0x98, 0x02, 0xcc, 0xdb, 0x8a, 0x95, 0x2d, 0x32, 0x00, 0xe5, 0x29, 0xe3, 0x28, 0x4f, 0x60, 0xd7},
	{0x6e, 0xe9, 0x81, 0xee, 0x15, 0x26, 0x6b, 0xe1, 0x3f, 0xb8, 0x32, 0x6a, 0x1b, 0x21, 0x03, 0xfc, 0x67, 0xd0, 0xc2, 0xc1},
	{0x2a, 0x07, 0x4e, 0xde, 0xbb, 0xef, 0x89, 0x9c, 0x98, 0xbb, 0xa5, 0xd5, 0xd8, 0x62, 0xc7, 0x1b, 0xa5, 0x50, 0x62, 0x62},
	{0xc3, 0x91, 0x15, 0x13, 0xb3, 0xa5, 0x78, 0x36, 0x82, 0x71, 0x21, 0x74, 0x4d, 0xc9, 0x82, 0x84, 0xf7, 0xcf, 0x98, 0x44},
	{0x2f, 0x24, 0x78, 0x27, 0x1e, 0x09, 0xa4, 0xe0, 0xff, 0x7a, 0x41, 0x3f, 0xe9, 0x59, 0xaf, 0x44, 0x69, 0x9a, 0x61, 0xfa},
	{0xa8, 0x5f, 0x8b, 0xa6, 0xc4, 0x94, 0x1b, 0xee, 0x51, 0x98, 0x61, 0x8a, 0x3d, 0x6a, 0xbe, 0x0d, 0xdf, 0xf2, 0x10, 0x0c},
	{0xda, 0x22, 0x25, 0x5a, 0x81, 0x62, 0x49, 0x7c, 0x85, 0xe7, 0xe6, 0x03, 0x91, 0x51, 0x9f, 0x32, 0x79, 0xd5, 0x5f, 0x1f},
	{0x20, 0xdf, 0x76, 0xc4, 0x0a, 0x29, 0x37, 0x09, 0x26, 0x05, 0xea, 0xbb, 0xa7, 0xb6, 0x29, 0xb6, 0x75, 0xba, 0x03, 0xbe},
	{0xc1, 0x83, 0x8a, 0x65, 0xce, 0xb1, 0x89, 0x40, 0x05, 0xb3, 0x6f, 0x60, 0x71, 0xff, 0x67, 0x46, 0xad, 0x71, 0xb3, 0x2e},
	{0x58, 0xb1, 0xec, 0x5f, 0xee, 0x7d, 0xd1, 0xa7, 0x61, 0xed, 0x90, 0x1b, 0x37, 0x4c, 0xcb, 0x97, 0x87, 0x37, 0xa9, 0x79},
	{0x26, 0xa2, 0x8f, 0xdd, 0xe7, 0xff, 0xc6, 0xcb, 0xdb, 0x4d, 0x01, 0x93, 0x4a, 0x1b, 0x69, 0xb7, 0x38, 0xb2, 0xb9, 0xfc},
	{0x5a, 0xcd, 0x25, 0x7a, 0x91, 0xfa, 0x8c, 0x95, 0x96, 0x8a, 0xf6, 0xea, 0xce, 0xfa, 0x73, 0x30, 0x69, 0x6a, 0x8f, 0x9d},
	{0x40, 0x45, 0xd8, 0xa0, 0x07, 0x06, 0x6a, 0x03, 0x2c, 0xda, 0xca, 0xdd, 0x09, 0x7d, 0x77, 0x11, 0x7b, 0x82, 0x13, 0xf9},
	{0x3d, 0xb8, 0xa0, 0x60, 0x71, 0x52, 0xa8, 0x89, 0xb6, 0x40, 0x65, 0x17, 0xde, 0x4a, 0x7d, 0xaf, 0x73, 0xb0, 0xa8, 0x52},
	{0x91, 0xaa, 0xb8, 0x7c, 0x37, 0xf4, 0x1a, 0xdf, 0x30, 0xea, 0x92, 0x49, 0xa6, 0x83, 0xbf, 0x89, 0xd8, 0x7e, 0x71, 0x27},
	{0xa5, 0x62, 0x72, 0xca, 0x3b, 0x9e, 0x64, 0xd2, 0xaa, 0xb2, 0x01, 0xbb, 0xbc, 0x6e, 0x56, 0xa5, 0x64, 0xd1, 0x7b, 0x44},
	{0x55, 0xd4, 0x77, 0xda, 0x8b, 0x1c, 0x60, 0x75, 0xeb, 0x35, 0x13, 0x73, 0xba, 0xec, 0xa0, 0x6a, 0x71, 0xa0, 0x63, 0xc0},
	{0xbf, 0xd0, 0xa9, 0xa0, 0x43, 0xba, 0x17, 0x7f, 0x75, 0x55, 0x22, 0x1f, 0xaf, 0x31, 0xcd, 0xa2, 0x43, 0x3f, 0xd8, 0x95},
	{0x9f, 0xe5, 0xd3, 0xdb, 0x8f, 0xb4, 0xb5, 0x47, 0xc4, 0xd7, 0xb9, 0xd6, 0xc9, 0x6f, 0x64, 0x23, 0x32, 0x13, 0x28, 0x3b},
	{0x48, 0xb3, 0xae, 0x8d, 0x27, 0xd8, 0x13, 0x8b, 0x5b, 0x47, 0x05, 0x2d, 0x2f, 0x81, 0x84, 0xbf, 0x55, 0x5a, 0xd1, 0x8e},
	{0x68, 0x83, 0xee, 0xbc, 0xa7, 0x76, 0x5a, 0x4c, 0x3e, 0xfc, 0x9d, 0xe0, 0xed, 0x15, 0x62, 0x41, 0x5e, 0x43, 0xf4, 0x35},
	{0x3d, 0xdc, 0x7c, 0xe4, 0x81, 0x08, 0x8b, 0xdc, 0x4b, 0x4f, 0x4f, 0xa9, 0x4b, 0x7a, 0x54, 0x20, 0x90, 0xf9, 0x4c, 0xa8},
	{0x91, 0xeb, 0xd7, 0x34, 0xb1, 0x63, 0x97, 0x2b, 0x05, 0x4c, 0x1d, 0x26, 0x7f, 0x03, 0x6b, 0x72, 0x2b, 0xd7, 0xe3, 0x53},
	{0xfc, 0xfb, 0xe4, 0xc6, 0x04, 0x4b, 0xa2, 0xff, 0xc6, 0xb3, 0x90, 0x0d, 0xa0, 0x84, 0x4c, 0x98, 0xbc, 0x2d, 0x06, 0xb0},
	{0x22, 0x5e, 0xa3, 0x49, 0xb9, 0xcb, 0x3b, 0x1b, 0x94, 0xe2, 0x37, 0xde, 0xb7, 0x97, 0xe0, 0xc6, 0x0d, 0x14, 0xa8, 0x4c},
	{0x2f, 0x3f, 0xb1, 0xcb, 0x59, 0x5b, 0x70, 0x65, 0x57, 0x08, 0x24, 0x16, 0xc3, 0xed, 0x83, 0x2b, 0xab, 0x94, 0x74, 0x6e},
	{0xff, 0xf2, 0xdf, 0x19, 0xf5, 0xf9, 0xf5, 0xfd, 0x87, 0x3e, 0xc7, 0xe8, 0xca, 0x3c, 0x4e, 0xbe, 0x20, 0xf2, 0x5b, 0x3e},
	{0xa9, 0x8a, 0x1c, 0x25, 0x68, 0xb8, 0x4b, 0xf3, 0x28, 0xec, 0xf7, 0x34, 0xe5, 0xef, 0x7a, 0x97, 0x79, 0x7c, 0x80, 0xea},
	{0x76, 0xb6, 0x41, 0x37, 0x5d, 0x13, 0x6c, 0x08, 0xf5, 0xfe, 0xb4, 0x6a, 0xac, 0xeb, 0xee, 0x40, 0x46, 0x8a, 0xc0, 0x85},
	{0x62, 0x90, 0x30, 0x07, 0x07, 0x56, 0xf8, 0xc5, 0x35, 0xce, 0x2c, 0x4a, 0x61, 0x20, 0xee, 0x52, 0xa1, 0xd1, 0x7b, 0x38},
	{0x09, 0x9d, 0x26, 0xf8, 0xf9, 0x29, 0xf8, 0xc0, 0x8e, 0x5f, 0x38, 0x8e, 0x3d, 0xc5, 0x9b, 0x71, 0xa7, 0x10, 0xbc, 0x9b},
	{0x98, 0xe2, 0xd9, 0xfb, 0x5d, 0xea, 0x02, 0x5b, 0xb3, 0x74, 0xe5, 0x72, 0xec, 0xa1, 0x75, 0x5a, 0x09, 0x09, 0x5b, 0x8d},
	{0x07, 0xa5, 0x16, 0xde, 0x13, 0xf7, 0x6a, 0xa0, 0xe5, 0x13, 0x66, 0xb0, 0xff, 0x80, 0x95, 0x89, 0x44, 0xa4, 0xb8, 0x2e},
	{0xfb, 0x17, 0xb4, 0xba, 0x5f, 0x42, 0x5d, 0xaf, 0x6b, 0x18, 0xe8, 0x88, 0xe0, 0xb1, 0x7a, 0x9f, 0x53, 0xee, 0xe4, 0xef},
	{0x9f, 0x5a, 0x56, 0x3a, 0x1e, 0x87, 0x64, 0x43, 0xd8, 0xbc, 0xd5, 0xd5, 0xf7, 0xdf, 0xdf, 0x4f, 0x73, 0x1b, 0xfb, 0x9a},
	{0x58, 0x7e, 0x51, 0xfc, 0xda, 0xcb, 0xe9, 0xce, 0xb4, 0x56, 0x49, 0x7b, 0x77, 0x6f, 0x7c, 0x05, 0x58, 0xb8, 0xb1, 0xed},
	{0xbf, 0x68, 0xa1, 0x8c, 0x2c, 0x4b, 0x14, 0x7f, 0xb5, 0xac, 0xbf, 0x42, 0x9d, 0x87, 0xdb, 0x98, 0xe4, 0xb6, 0xfc, 0x57},
	{0x3b, 0xe6, 0x98, 0xc7, 0x7c, 0x2b, 0xfb, 0xc7, 0x26, 0xfd, 0xf9, 0xbb, 0xc1, 0xf5, 0x20, 0xe6, 0x1a, 0xf7, 0xc8, 0xf4},
	{0x1c, 0x9e, 0x13, 0x8e, 0xa7, 0xf6, 0x61, 0xa9, 0xc3, 0x95, 0x11, 0x4a, 0xd2, 0xb1, 0x65, 0x7b, 0x93, 0x39, 0x6b, 0x85},
	{0x63, 0x7e, 0x40, 0x17, 0x6f, 0x07, 0x6f, 0x5e, 0x6f, 0xee, 0xcc, 0x3e, 0x78, 0x8d, 0x33, 0xaa, 0x0b, 0xe2, 0xad, 0x79},
	{0x82, 0x76, 0x5a, 0xf3, 0x5f, 0xd0, 0x3a, 0x57, 0x7f, 0x5f, 0x03, 0x03, 0x3a, 0x1d, 0x84, 0x86, 0xc3, 0x1b, 0x2d, 0x01},
	{0x77, 0x76, 0xea, 0x67, 0x86, 0xe6, 0xe4, 0xc0, 0x3d, 0x53, 0xac, 0xb3, 0x9d, 0x7e, 0x2d, 0x13, 0xf0, 0x14, 0xa0, 0x68},
	{0x39, 0xe7, 0x59, 0xd1, 0x60, 0x82, 0xe2, 0xa2, 0x3f, 0x5b, 0xd3, 0xea, 0xcc, 0xee, 0xe3, 0xb5, 0x35, 0x97, 0x58, 0xb0},
	{0x85, 0x92, 0xb1, 0x47, 0xab, 0xc0, 0x35, 0x6e, 0x7a, 0x8f, 0x19, 0x9e, 0x3e, 0x5f, 0x8b, 0x90, 0x34, 0x62, 0xd6, 0xfc},
	{0x48, 0x70, 0x94, 0x85, 0xcb, 0xd2, 0x72, 0x9e, 0x96, 0xd8, 0xd0, 0x8e, 0xcc, 0x31, 0x77, 0xad, 0x57, 0xfd, 0x95, 0x2b},
	{0x31, 0x61, 0xe2, 0xb4, 0x7f, 0xaf, 0xc3, 0x1f, 0x78, 0xda, 0xa8, 0xca, 0xd2, 0x27, 0x2a, 0x87, 0xb4, 0x00, 0x0c, 0xc1},
	{0x9e, 0x18, 0x1b, 0x62, 0xc6, 0xf2, 0xe2, 0xeb, 0xea, 0x81, 0xcd, 0xbf, 0x87, 0xc4, 0x8f, 0x36, 0x9f, 0xf3, 0x72, 0x41},
	{0x45, 0xe0, 0x46, 0xa2, 0x2e, 0xb8, 0x6f, 0xdc, 0xde, 0x60, 0xae, 0x17, 0x82, 0x23, 0xb2, 0x6f, 0x5b, 0x57, 0xb5, 0xde},
	{0xfa, 0xdb, 0x75, 0xaa, 0xf5, 0x19, 0xa9, 0x79, 0x29, 0x29, 0x44, 0x3d, 0x9d, 0xfd, 0x52, 0x5e, 0xe5, 0x9a, 0x1b, 0xf9},
	{0x29, 0xf2, 0xc4, 0x00, 0x1a, 0x87, 0xa7, 0x39, 0x4e, 0x58, 0xad, 0xd0, 0x29, 0xef, 0x9b, 0xc9, 0x9e, 0x69, 0x2c, 0x27},
	{0x8a, 0x8b, 0x42, 0x5c, 0xa1, 0xf0, 0x65, 0x59, 0x3f, 0x37, 0x98, 0xdd, 0x85, 0xae, 0xb2, 0xf2, 0x8b, 0x80, 0xfa, 0xfa},
	{0xa2, 0xd8, 0xc8, 0x8f, 0x41, 0x64, 0xb2, 0xd3, 0x83, 0xa3, 0x4e, 0x4c, 0x7d, 0xe5, 0x22, 0xd6, 0x71, 0xe1, 0x55, 0x0e},
	{0x2f, 0xbb, 0xe6, 0x60, 0xee, 0xa8, 0xac, 0x59, 0xdc, 0x5e, 0x76, 0xb4, 0xcb, 0x2f, 0xa7, 0x35, 0x98, 0xd2, 0x4c, 0x0f},
	{0x53, 0xf8, 0x9f, 0xab, 0xb6, 0x78, 0x78, 0xbd, 0x3b, 0x63, 0x45, 0x91, 0x56, 0xee, 0x7b, 0x5a, 0x97, 0x67, 0x7b, 0x7a},
	{0x31, 0x95, 0xe4, 0x1d, 0x32, 0xf5, 0x4c, 0x19, 0xed, 0xb5, 0xd3, 0xfe, 0x2b, 0x3b, 0x25, 0xd0, 0xcc, 0xad, 0x08, 0x77},
	{0x7e, 0xf8, 0x10, 0xfd, 0x6c, 0x79, 0xc2, 0x38, 0x10, 0xa1, 0x50, 0x9e, 0x3a, 0x13, 0x9c, 0xb3, 0xb7, 0x50, 0x21, 0xb6},
	{0xbc, 0x06, 0xc0, 0x72, 0x13, 0x82, 0x1f, 0x94, 0x7c, 0xd7, 0x59, 0x6b, 0x61, 0xa9, 0x03, 0xc3, 0xe8, 0xbc, 0xd0, 0x9d},
	{0xc7, 0xbc, 0x12, 0xec, 0x0c, 0xa9, 0x35, 0x70, 0xc5, 0x31, 0x6e, 0xb4, 0xb0, 0xcf, 0x3c, 0xc6, 0xcc, 0x60, 0x24, 0xa0},
};

static hpt_pa_t ucode_host_ctx_ptr2pa(void *vctx, void *ptr)
{
	(void)vctx;
	return hva2spa(ptr);
}

static void* ucode_host_ctx_pa2ptr(void *vctx, hpt_pa_t spa, size_t sz, hpt_prot_t access_type, hptw_cpl_t cpl, size_t *avail_sz)
{
	(void)vctx;
	(void)access_type;
	(void)cpl;
	*avail_sz = sz;
	return spa2hva(spa);
}

static hpt_pa_t ucode_guest_ctx_ptr2pa(void __attribute__((unused)) *ctx, void *ptr)
{
	return hva2gpa(ptr);
}

static void* ucode_guest_ctx_pa2ptr(void *vctx, hpt_pa_t gpa, size_t sz, hpt_prot_t access_type, hptw_cpl_t cpl, size_t *avail_sz)
{
	ucode_hptw_ctx_pair_t *ctx_pair = vctx;

	return hptw_checked_access_va(&ctx_pair->host_ctx,
									access_type,
									cpl,
									gpa,
									sz,
									avail_sz);
}

static void* ucode_ctx_unimplemented(void *vctx, size_t alignment, size_t sz)
{
	(void)vctx;
	(void)alignment;
	(void)sz;
	HALT_ON_ERRORCOND(0 && "Not implemented");
	return NULL;
}

/*
 * Check SHA-1 hash of the update
 * Return 1 if the update is recognized, 0 otherwise
 */
static int ucode_check_sha1(intel_ucode_update_t *header)
{
	const unsigned char *buffer = (const unsigned char *) header;
	unsigned char md[SHA_DIGEST_LENGTH];
	HALT_ON_ERRORCOND(sha1_buffer(buffer, header->total_size, md) == 0);
	print_hex("SHA1(update) = ", md, SHA_DIGEST_LENGTH);
	for (u32 i = 0; i < sizeof(ucode_recognized_sha1s) / sizeof(ucode_recognized_sha1s[0]); i++) {
		if (memcmp(md, ucode_recognized_sha1s[i], SHA_DIGEST_LENGTH) == 0) {
			return 1;
		}
	}
	return 0;
}

/*
 * Check the processor flags of the update
 * Return 1 if the update is for this processor, 0 otherwise
 */
static int ucode_check_processor_flags(u32 processor_flags)
{
	u64 flag = 1 << ((rdmsr64(IA32_PLATFORM_ID) >> 50) & 0x7);
	return !!(processor_flags & flag);
}

/*
 * Check the processor signature and processor flags of the update
 * Return 1 if the update is for this processor, 0 otherwise
 */
static int ucode_check_processor(intel_ucode_update_t *header)
{
	u32 eax, ebx, ecx, edx;
	cpuid(1, &eax, &ebx, &ecx, &edx);
	HALT_ON_ERRORCOND(header->header_version == 1);
	if (header->processor_signature == eax) {
		return ucode_check_processor_flags(header->processor_flags);
	} else if (header->total_size > header->data_size + 48) {
		intel_ucode_ext_sign_table_t *ext_sign_table;
		u32 n;
		u32 ext_size = header->total_size - (header->data_size + 48);
		HALT_ON_ERRORCOND(ext_size >= sizeof(intel_ucode_ext_sign_table_t));
		ext_sign_table = ((void *) header) + (header->data_size + 48);
		n = ext_sign_table->extended_signature_count;
		HALT_ON_ERRORCOND(ext_size >= sizeof(intel_ucode_ext_sign_table_t) +
							n * sizeof(intel_ucode_ext_sign_t));
		for (u32 i = 0; i < n; i++) {
			intel_ucode_ext_sign_t *signature = &ext_sign_table->signatures[i];
			if (signature->processor_signature == eax) {
				return ucode_check_processor_flags(signature->processor_flags);
			}
		}
	}
	return 0;
}

/*
 * Update microcode.
 * This function should be called when handling WRMSR to IA32_BIOS_UPDT_TRIG.
 * update_data should be EDX:EAX, which is the guest virtual address of "Update
 * Data". The header and other metadata (e.g. size) starts at 48 bytes before
 * this address.
 */
void handle_intel_ucode_update(VCPU *vcpu, u64 update_data)
{
	hpt_type_t guest_t = hpt_emhf_get_guest_hpt_type(vcpu);
	ucode_hptw_ctx_pair_t ctx_pair = {
		.guest_ctx = {
			.ptr2pa = ucode_guest_ctx_ptr2pa,
			.pa2ptr = ucode_guest_ctx_pa2ptr,
			.gzp = ucode_ctx_unimplemented,
			.root_pa = hpt_cr3_get_address(guest_t, vcpu->vmcs.guest_CR3),
			.t = guest_t,
		},
		.host_ctx = {
			.ptr2pa = ucode_host_ctx_ptr2pa,
			.pa2ptr = ucode_host_ctx_pa2ptr,
			.gzp = ucode_ctx_unimplemented,
			.root_pa = hpt_eptp_get_address(HPT_TYPE_EPT,
											vcpu->vmcs.control_EPT_pointer),
			.t = HPT_TYPE_EPT,
		}
	};
	hptw_ctx_t *ctx = &ctx_pair.guest_ctx;
	u64 va_header = update_data - sizeof(intel_ucode_update_t);
	u8 *copy_area;
	intel_ucode_update_t *header;
	int result;
	size_t size;
	HALT_ON_ERRORCOND(vcpu->idx < UCODE_TOTAL_SIZE_MAX);
	copy_area = ucode_copy_area + UCODE_TOTAL_SIZE_MAX * vcpu->idx;
	/* Copy header of microcode update */
	header = (intel_ucode_update_t *) copy_area;
	size = sizeof(intel_ucode_update_t);
	result = hptw_checked_copy_from_va(ctx, 0, header, va_header, size);
	HALT_ON_ERRORCOND(result == 0);
	printf("\nCPU(0x%02x): date(mmddyyyy)=%08x, dsize=%d, tsize=%d",
			vcpu->id, header->date, header->data_size, header->total_size);
	/* If the following check fails, increase UCODE_TOTAL_SIZE_MAX */
	HALT_ON_ERRORCOND(header->total_size <= UCODE_TOTAL_SIZE_MAX);
	/* Copy the rest of of microcode update */
	size = header->total_size - size;
	result = hptw_checked_copy_from_va(ctx, 0, &header->update_data,
										update_data, size);
	/* Check the hash of the update */
	if (!ucode_check_sha1(header)) {
		printf("\nCPU(0x%02x): Unrecognized microcode update, HALT!", vcpu->id);
		HALT();
	}
	/* Check whether update is for the processor */
	if (!ucode_check_processor(header)) {
		printf("\nCPU(0x%02x): Incompatible microcode update, HALT!", vcpu->id);
		HALT();
	}
	/*
	for (u32 i = 0; i < header->total_size; i++) {
		if (i % 16 == 0) {
			printf("\n%08x  ", i);
		} else if (i % 8 == 0) {
			printf("  ");
		} else {
			printf(" ");
		}
		printf("%02x", copy_area[i]);
	}
	*/
	/* Forward microcode update to host */
	printf("\nCPU(0x%02x): Calling physical ucode update at 0x%08lx",
			vcpu->id, &header->update_data);
	wrmsr64(IA32_BIOS_UPDT_TRIG, (uintptr_t) &header->update_data);
	printf("\nCPU(0x%02x): Physical ucode update returned", vcpu->id);
}

