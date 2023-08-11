/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <xmhf.h>

u64 g_gdt[MAX_VCPU_ENTRIES][XMHF_GDT_SIZE];

void init_gdt(VCPU * vcpu)
{
	g_gdt[vcpu->idx][0] = 0x0000000000000000ULL;
	g_gdt[vcpu->idx][1] = 0x00cf9a000000ffffULL;	// CS 32-bit
	g_gdt[vcpu->idx][2] = 0x00cf92000000ffffULL;	// DS
	g_gdt[vcpu->idx][3] = 0x00af9a000000ffffULL;	// CS 64-bit
	g_gdt[vcpu->idx][4] = 0x0000000000000000ULL;	// TSS
	g_gdt[vcpu->idx][5] = 0x0000000000000000ULL;	// TSS (only used in 64-bit)
	g_gdt[vcpu->idx][6] = 0x0000000000000000ULL;	// CS Ring 3
	g_gdt[vcpu->idx][7] = 0x0000000000000000ULL;	// DS Ring 3
	g_gdt[vcpu->idx][8] = 0x0000000000000000ULL;
	g_gdt[vcpu->idx][9] = 0x0000000000000000ULL;

	struct {
		u16 limit;
		uintptr_t base;
	} __attribute__((packed)) gdtr = {
		.limit=XMHF_GDT_SIZE * 8 - 1,
		.base=(uintptr_t)&(g_gdt[vcpu->idx][0]),
	};
	asm volatile("lgdt %0" : : "m"(gdtr));
}
