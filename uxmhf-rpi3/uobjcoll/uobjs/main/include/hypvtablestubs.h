/*
 * @UBERXMHF_LICENSE_HEADER_START@
 *
 * uber eXtensible Micro-Hypervisor Framework (Raspberry Pi)
 *
 * Copyright 2018 Carnegie Mellon University. All Rights Reserved.
 *
 * NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
 * INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
 * UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 * AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR
 * PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF
 * THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF
 * ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.
 *
 * Released under a BSD (SEI)-style license, please see LICENSE or
 * contact permission@sei.cmu.edu for full terms.
 *
 * [DISTRIBUTION STATEMENT A] This material has been approved for public
 * release and unlimited distribution.  Please see Copyright notice for
 * non-US Government use and distribution.
 *
 * Carnegie Mellon is registered in the U.S. Patent and Trademark Office by
 * Carnegie Mellon University.
 *
 * @UBERXMHF_LICENSE_HEADER_END@
 */

/*
 * Author: Amit Vasudevan (amitvasudevan@acm.org)
 *
 */

/*
	entry stub

	author: amit vasudevan (amitvasudevan@acm.org), ethan joseph (ethanj217@gmail.com)
*/

#ifndef __HYPVTABLESTUBS_H__
#define __HYPVTABLESTUBS_H__

#include <uberspark/uobjcoll/platform/rpi3/uxmhf/uobjs/main/include/types.h>

#ifndef __ASSEMBLY__


extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_stack[8192 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_hypsvc_stack0[16384 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_hypsvc_stack1[16384 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_hypsvc_stack2[16384 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_hypsvc_stack3[16384 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_rsvhandler_stack0[8192 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_rsvhandler_stack1[8192 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_rsvhandler_stack2[8192 / 4];

extern __attribute__((section(".stack"))) __attribute__((aligned(8))) u32 hypvtable_rsvhandler_stack3[8192 / 4];

#endif // __ASSEMBLY__

#endif // __HYPVTABLESTUBS_H__
