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

// peh-x86vmx-emulation.c
// Emulate selected x86 instruction
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>

#define INST_LEN_MAX	15
#define BIT_SIZE_64		8
#define BIT_SIZE_32		4
#define BIT_SIZE_16		2
#define BIT_SIZE_8		1

/* Select a segment that will be used to access memory */
typedef enum cpu_segment_t {
	CPU_SEG_ES,
	CPU_SEG_CS,
	CPU_SEG_SS,
	CPU_SEG_DS,
	CPU_SEG_FS,
	CPU_SEG_GS,
	CPU_SEG_UNKNOWN,
} cpu_segment_t;

/* Environment used to access memory */
typedef struct mem_access_env_t {
	void *haddr;
	gva_t gaddr;
	cpu_segment_t seg;
	size_t size;
	hpt_prot_t mode;
	hptw_cpl_t cpl;
} mem_access_env_t;

/* Information about instruction prefixes */
typedef struct prefix_t {
	bool lock;
	bool repe;
	bool repne;
	cpu_segment_t seg;
	bool opsize;
	bool addrsize;
	union {
		struct {
			u8 b : 1;
			u8 x : 1;
			u8 r : 1;
			u8 w : 1;
			u8 four : 4;
		};
		u8 raw;
	} rex;
} prefix_t;

#define PREFIX_INFO_INITIALIZER \
	{ false, false, false, CPU_SEG_UNKNOWN, false, false, { .raw=0 } }

/* ModR/M */
typedef union modrm_t {
	struct {
		u8 rm : 3;
		u8 regop : 3;
		u8 mod : 2;
	};
	u8 raw;
} modrm_t;

/* SIB */
typedef union sib_t {
	struct {
		u8 base : 3;
		u8 index : 3;
		u8 scale : 2;
	};
	u8 raw;
} sib_t;

/* Instruction postfixes (bytes after opcode) */
typedef struct postfix_t {
	modrm_t modrm;
	sib_t sib;
	union {
		unsigned char *displacement;
		u8 *displacement1;
		u16 *displacement2;
		u32 *displacement4;
		u64 *displacement8;
	};
	union {
		unsigned char *immediate;
		u8 *immediate1;
		u16 *immediate2;
		u32 *immediate4;
		u64 *immediate8;
	};
} postfix_t;

typedef struct emu_env_t {
	VCPU * vcpu;
	struct regs *r;
	guestmem_hptw_ctx_pair_t ctx_pair;
	bool g64;
	bool cs_d;
	u8 *pinst;
	u32 pinst_len;
	prefix_t prefix;
	postfix_t postfix;
	cpu_segment_t seg;
	size_t displacement_len;
	size_t immediate_len;
} emu_env_t;

/*
 * Handle access of special memory.
 * Interface similar to hptw_checked_access_va.
 */
static void access_special_memory(VCPU * vcpu, void *hva,
								  hpt_prot_t access_type, hptw_cpl_t cpl,
								  gpa_t gpa, size_t requested_sz,
								  size_t *avail_sz)
{
	if ((gpa & PAGE_MASK_4K) == g_vmx_lapic_base) {
		HALT_ON_ERRORCOND(requested_sz == 4);
		HALT_ON_ERRORCOND(gpa % 4 == 0);
		HALT_ON_ERRORCOND(cpl == 0);
		*avail_sz = 4;
		switch (gpa & ADDR64_PAGE_OFFSET_4K) {
		case LAPIC_ICR_LOW:
			xmhf_smpguest_arch_x86vmx_eventhandler_icrlowwrite(vcpu,
															   *(u32 *)hva);
			break;
		default:
			if (access_type & HPT_PROT_WRITE_MASK) {
				*(u32 *)(uintptr_t)gpa = *(u32 *)hva;
			} else {
				*(u32 *)hva = *(u32 *)(uintptr_t)gpa;
			}
			break;
		}
	} else {
		HALT_ON_ERRORCOND(0);
	}
}

/*
 * Given a segment index, translate logical address to linear address.
 * seg: index of segment used by hardware.
 * addr: logical address as input, modified to linear address as output.
 * size: size of access.
 * mode: access mode (read / write / execute). Use HPT_PROT_*_MASK macros.
 * cpl: permission of access.
 *
 * Note: currently XMHF halts when guest makes an invalid access. Ideally XMHF
 * should inject corresponding exceptions like #GP(0).
 *
 * TODO: merge this function with nested-x86vmx-handler1.c
 */
static gva_t _vmx_decode_seg(VCPU * vcpu, cpu_segment_t seg, gva_t addr,
							 size_t size, hpt_prot_t mode, hptw_cpl_t cpl)
{
	/* Get segment fields from VMCS */
	ulong_t base;
	u32 access_rights;
	u32 limit;
	bool g64 = VCPU_g64(vcpu);
	switch (seg) {
	case CPU_SEG_ES:
		base = vcpu->vmcs.guest_ES_base;
		access_rights = vcpu->vmcs.guest_ES_access_rights;
		limit = vcpu->vmcs.guest_ES_limit;
		break;
	case CPU_SEG_CS:
		base = vcpu->vmcs.guest_CS_base;
		access_rights = vcpu->vmcs.guest_CS_access_rights;
		limit = vcpu->vmcs.guest_CS_limit;
		break;
	case CPU_SEG_SS:
		base = vcpu->vmcs.guest_SS_base;
		access_rights = vcpu->vmcs.guest_SS_access_rights;
		limit = vcpu->vmcs.guest_SS_limit;
		break;
	case CPU_SEG_DS:
		base = vcpu->vmcs.guest_DS_base;
		access_rights = vcpu->vmcs.guest_DS_access_rights;
		limit = vcpu->vmcs.guest_DS_limit;
		break;
	case CPU_SEG_FS:
		base = vcpu->vmcs.guest_FS_base;
		access_rights = vcpu->vmcs.guest_FS_access_rights;
		limit = vcpu->vmcs.guest_FS_limit;
		break;
	case CPU_SEG_GS:
		base = vcpu->vmcs.guest_GS_base;
		access_rights = vcpu->vmcs.guest_GS_access_rights;
		limit = vcpu->vmcs.guest_GS_limit;
		break;
	default:
		HALT_ON_ERRORCOND(0 && "Unexpected segment");
	}
	/*
	 * For 32-bit guest, check segment limit.
	 * For 64-bit guest, no limit check is performed (can even wrap around).
	 */
	if (!g64) {
		gva_t addr_max;
		HALT_ON_ERRORCOND(addr <= UINT_MAX);
		if (addr > UINT_MAX - size) {
			HALT_ON_ERRORCOND(0 && "Invalid access: logical address overflow");
		}
		addr_max = addr + size;
		if (addr_max >= limit) {
			HALT_ON_ERRORCOND(0 && "Invalid access: segment limit exceed");
		}
		if (base > UINT_MAX - addr_max) {
			HALT_ON_ERRORCOND(0 && "Not implemented: linear address overflow");
		}
	}
	/* Check access rights. Skip checking if amd64 SS/DS/ES/FS/GS. */
	if (seg == 1 || !g64) {
		/* Check Segment type */
		{
			hpt_prot_t supported_modes = HPT_PROTS_NONE;
			if ((access_rights & (1U << 3)) == 0) {
				/* type = 0 - 7, always has read */
				supported_modes |= HPT_PROT_READ_MASK;
				if (access_rights & (1U << 1)) {
					/* type = 2/3/6/7, has write */
					supported_modes |= HPT_PROT_WRITE_MASK;
				}
				if (access_rights & (1U << 2)) {
					/* type = 4 - 7, expand-down data segment not implemented */
					HALT_ON_ERRORCOND(0 && "Not implemented: expand-down");
				}
			} else {
				/* type = 8 - 15, always has execute */
				supported_modes |= HPT_PROT_EXEC_MASK;
				if (access_rights & (1U << 1)) {
					/* type = 10/11/14/15, has read */
					supported_modes |= HPT_PROT_READ_MASK;
				}
			}
			if ((supported_modes & mode) != mode) {
				HALT_ON_ERRORCOND(0 && "Invalid access: segment type");
			}
		}
		/* Check S - Descriptor type (0 = system; 1 = code or data) */
		HALT_ON_ERRORCOND((access_rights & (1U << 4)));
		/* Check DPL - Descriptor privilege level */
		{
			u32 dpl = (access_rights >> 5) & 0x3;
			if (dpl < (u32) cpl) {
				HALT_ON_ERRORCOND(0 && "Invalid access: DPL");
			}
		}
		/* Check P - Segment present */
		HALT_ON_ERRORCOND((access_rights & (1U << 7)));
	}
	return addr + base;
}

/* Access memory from guest logical address. */
static void access_memory_gv(guestmem_hptw_ctx_pair_t * ctx_pair,
							 mem_access_env_t * env)
{
	VCPU *vcpu = ctx_pair->vcpu;
	hpt_va_t lin_addr;
	size_t copied;
	memprot_x86vmx_eptlock_read_lock(vcpu);

	/* Segmentation: logical address -> linear address */
	lin_addr = _vmx_decode_seg(vcpu, env->seg, env->gaddr, env->size, env->mode,
							   env->cpl);

	/* Paging */
	copied = 0;
	while (copied < env->size) {
		hpt_va_t gva = lin_addr + copied;
		size_t size = env->size - copied;
		size_t old_size;
		hpt_va_t gpa;
		void *hva;

		/* Linear address -> guest physical address */
		if (vcpu->vmcs.guest_CR0 & CR0_PG) {
			hpt_pmeo_t pmeo;
			int ret = hptw_checked_get_pmeo(&pmeo, &ctx_pair->guest_ctx,
										    env->mode, env->cpl, gva);
			HALT_ON_ERRORCOND(ret == 0);
			gpa = hpt_pmeo_va_to_pa(&pmeo, gva);
			size = MIN(size, hpt_remaining_on_page(&pmeo, gpa));
		} else {
			gpa = gva;
		}

		/* Guest physical address -> hypervisor physical address */
		old_size = size;
		hva = hptw_checked_access_va(&ctx_pair->host_ctx, env->mode, env->cpl,
									 gpa, size, &size);
		if (hva == NULL) {
			/* Memory not in EPT, need special treatment */
			access_special_memory(vcpu, env->haddr + copied, env->mode,
								  env->cpl, gpa, old_size, &size);
			copied += size;
		} else {
			/* Perform normal memory access */
			if (env->mode & HPT_PROT_WRITE_MASK) {
				memcpy(hva, env->haddr + copied, size);
			} else {
				memcpy(env->haddr + copied, hva, size);
			}
			copied += size;
		}
	}

	memprot_x86vmx_eptlock_read_unlock(vcpu);
}

#define EXTEND(dtype, stype) \
	do { \
		_Static_assert(sizeof(dtype) >= sizeof(stype), "Type size mismatch"); \
		if (sizeof(dtype) == dst_size && sizeof(stype) == src_size) { \
			*(dtype *)dst = (dtype)*(stype *)src; \
			return; \
		} \
	} while (0)

static void zero_extend(void *dst, void *src, size_t dst_size, size_t src_size)
{
	EXTEND(uint8_t, uint8_t);
	EXTEND(uint16_t, uint8_t);
	EXTEND(uint32_t, uint8_t);
	EXTEND(uint64_t, uint8_t);
	EXTEND(uint16_t, uint16_t);
	EXTEND(uint32_t, uint16_t);
	EXTEND(uint64_t, uint16_t);
	EXTEND(uint32_t, uint32_t);
	EXTEND(uint64_t, uint32_t);
	EXTEND(uint64_t, uint64_t);
	HALT_ON_ERRORCOND(0 && "Unknown sizes");
}

static void sign_extend(void *dst, void *src, size_t dst_size, size_t src_size)
{
	EXTEND(int8_t, int8_t);
	EXTEND(int16_t, int8_t);
	EXTEND(int32_t, int8_t);
	EXTEND(int64_t, int8_t);
	EXTEND(int16_t, int16_t);
	EXTEND(int32_t, int16_t);
	EXTEND(int64_t, int16_t);
	EXTEND(int32_t, int32_t);
	EXTEND(int64_t, int32_t);
	EXTEND(int64_t, int64_t);
	HALT_ON_ERRORCOND(0 && "Unknown sizes");
}

#undef EXTEND

/* Given register index, return its pointer */
static void *get_reg_ptr(emu_env_t * emu_env, enum CPU_Reg_Sel index,
						 size_t size)
{
	if (size == BIT_SIZE_8) {
		HALT_ON_ERRORCOND(0 && "Not implemented");
		// Note: in 32-bit mode, AH CH DH BH are different
		// Note: in 64-bit mode, AH CH DH BH do not exist
	}
	switch (index) {
		case CPU_REG_AX: return &emu_env->r->eax;
		case CPU_REG_CX: return &emu_env->r->ecx;
		case CPU_REG_DX: return &emu_env->r->edx;
		case CPU_REG_BX: return &emu_env->r->ebx;
		case CPU_REG_SP: return &emu_env->vcpu->vmcs.guest_RSP;
		case CPU_REG_BP: return &emu_env->r->ebp;
		case CPU_REG_SI: return &emu_env->r->esi;
		case CPU_REG_DI: return &emu_env->r->edi;
#ifdef __AMD64__
		case CPU_REG_R8: return &emu_env->r->r8;
		case CPU_REG_R9: return &emu_env->r->r9;
		case CPU_REG_R10: return &emu_env->r->r10;
		case CPU_REG_R11: return &emu_env->r->r11;
		case CPU_REG_R12: return &emu_env->r->r12;
		case CPU_REG_R13: return &emu_env->r->r13;
		case CPU_REG_R14: return &emu_env->r->r14;
		case CPU_REG_R15: return &emu_env->r->r15;
#elif !defined(__I386__)
    #error "Unsupported Arch"
#endif /* !defined(__I386__) */
		default: HALT_ON_ERRORCOND(0 && "Invalid register");
	}
}

/* Return whether the operand size is 32 bits */
static size_t get_operand_size(emu_env_t * emu_env)
{
	if (emu_env->g64) {
		if (emu_env->prefix.rex.w) {
			return BIT_SIZE_64;
		} else {
			return emu_env->prefix.opsize ? BIT_SIZE_16 : BIT_SIZE_32;
		}
	} else {
		if (emu_env->prefix.opsize) {
			return emu_env->cs_d ? BIT_SIZE_16 : BIT_SIZE_32;
		} else {
			return emu_env->cs_d ? BIT_SIZE_32 : BIT_SIZE_16;
		}
	}
}

/* Return whether the operand size is 32 bits */
static size_t get_address_size(emu_env_t * emu_env)
{
	if (emu_env->g64) {
		return emu_env->prefix.addrsize ? BIT_SIZE_32 : BIT_SIZE_64;
	}
	if (emu_env->prefix.addrsize) {
		return emu_env->cs_d ? BIT_SIZE_16 : BIT_SIZE_32;
	} else {
		return emu_env->cs_d ? BIT_SIZE_32 : BIT_SIZE_16;
	}
}

/* Return reg of ModRM, adjusted by REX prefix */
static u8 get_modrm_reg(emu_env_t * emu_env)
{
	u8 ans = emu_env->postfix.modrm.regop;
	if (emu_env->prefix.rex.four) {
		ans |= emu_env->prefix.rex.r << 3;
	}
	return ans;
}

/* Return rm of ModRM, adjusted by REX prefix */
static u8 get_modrm_rm(emu_env_t * emu_env)
{
	u8 ans = emu_env->postfix.modrm.rm;
	if (emu_env->prefix.rex.four) {
		ans |= emu_env->prefix.rex.b << 3;
	}
	return ans;
}

/* Return index of SIB, adjusted by REX prefix */
static u8 get_sib_index(emu_env_t * emu_env)
{
	u8 ans = emu_env->postfix.sib.index;
	if (emu_env->prefix.rex.four) {
		ans |= emu_env->prefix.rex.x << 3;
	}
	return ans;
}

/* Return base of SIB, adjusted by REX prefix */
static u8 get_sib_base(emu_env_t * emu_env)
{
	u8 ans = emu_env->postfix.sib.base;
	if (emu_env->prefix.rex.four) {
		ans |= emu_env->prefix.rex.b << 3;
	}
	return ans;
}

/* Return hypervisor memory address containing the register */
static void *eval_modrm_reg(emu_env_t * emu_env)
{
	size_t operand_size = get_operand_size(emu_env);
	return get_reg_ptr(emu_env, get_modrm_reg(emu_env), operand_size);
}

/*
 * Compute segment used for memory reference.
 * Ref: SDM volume 1, Table 3-5. Default Segment Selection Rules.
 */
static void compute_segment(emu_env_t * emu_env, enum CPU_Reg_Sel index)
{
	HALT_ON_ERRORCOND(emu_env->seg == CPU_SEG_UNKNOWN);
	if (emu_env->prefix.seg != CPU_SEG_UNKNOWN) {
		emu_env->seg = emu_env->prefix.seg;
	} else if (index == CPU_REG_BP || index == CPU_REG_SP) {
		emu_env->seg = CPU_SEG_SS;
	} else {
		emu_env->seg = CPU_SEG_DS;
	}
}

/* Return the value encoded by SIB */
static uintptr_t eval_sib_addr(emu_env_t * emu_env)
{
	size_t address_size = get_address_size(emu_env);
	size_t id = get_sib_index(emu_env);
	size_t bs = get_sib_base(emu_env);
	uintptr_t scaled_index = 0;
	uintptr_t base = 0;
	HALT_ON_ERRORCOND(address_size != BIT_SIZE_16 && "Not implemented");

	if (id != CPU_REG_SP) {
		zero_extend(&scaled_index, get_reg_ptr(emu_env, id, address_size),
					sizeof(scaled_index), address_size);
		scaled_index <<= emu_env->postfix.sib.scale;
	}

	if (!(emu_env->postfix.modrm.mod == 0 && bs % 8 == CPU_REG_BP)) {
		zero_extend(&base, get_reg_ptr(emu_env, bs, address_size),
					sizeof(base), address_size);
		compute_segment(emu_env, bs);
	} else {
		compute_segment(emu_env, CPU_REG_AX);
	}

	return scaled_index + base;
}

/*
 * If memory, return true and store guest memory address to addr.
 * If register, return false and store hypervisor memory address to addr.
 */
static bool eval_modrm_addr(emu_env_t * emu_env, uintptr_t *addr)
{
	size_t address_size = get_address_size(emu_env);
	HALT_ON_ERRORCOND(address_size != BIT_SIZE_16 && "Not implemented");

	/* Register operand, simple case */
	if (emu_env->postfix.modrm.mod == 3) {
		size_t operand_size = get_operand_size(emu_env);
		void *ans = get_reg_ptr(emu_env, get_modrm_rm(emu_env), operand_size);
		*addr = (uintptr_t)ans;
		return false;
	}

	/* Compute register / SIB */
	if (get_modrm_rm(emu_env) % 8 == CPU_REG_SP) {
		*addr = eval_sib_addr(emu_env);
	} else if (get_modrm_rm(emu_env) % 8 == CPU_REG_BP &&
			   emu_env->postfix.modrm.mod == 0) {
		HALT_ON_ERRORCOND(address_size != BIT_SIZE_64 &&
						  "Not implemented (RIP relative addressing)");
		*addr = 0;
		compute_segment(emu_env, CPU_REG_AX);
	} else {
		u8 rm = get_modrm_rm(emu_env);
		zero_extend(addr, get_reg_ptr(emu_env, rm, address_size), sizeof(addr),
					address_size);
		compute_segment(emu_env, rm);
	}

	/* Compute displacement */
	if (emu_env->displacement_len) {
		uintptr_t displacement;
		sign_extend(&displacement, emu_env->postfix.displacement,
					sizeof(displacement), emu_env->displacement_len);
		*addr += displacement;
	}

	/* Truncate result to address size */
	zero_extend(addr, addr, sizeof(*addr), address_size);

	return true;
}

/*
 * Read prefixes of instruction.
 * prefix should be initialized with PREFIX_INFO_INITIALIZER.
 */
static void parse_prefix(emu_env_t * emu_env)
{
	/* Group 1 - 4 */
	bool read_prefix;
	do {
		HALT_ON_ERRORCOND(emu_env->pinst_len > 0);
		read_prefix = true;
		switch (emu_env->pinst[0]) {
			case 0x26: emu_env->prefix.seg = CPU_SEG_ES; break;
			case 0x2e: emu_env->prefix.seg = CPU_SEG_CS; break;
			case 0x36: emu_env->prefix.seg = CPU_SEG_SS; break;
			case 0x3e: emu_env->prefix.seg = CPU_SEG_DS; break;
			case 0x64: emu_env->prefix.seg = CPU_SEG_FS; break;
			case 0x65: emu_env->prefix.seg = CPU_SEG_GS; break;
			case 0x66: emu_env->prefix.opsize = true; break;
			case 0x67: emu_env->prefix.addrsize = true; break;
			case 0xf0: emu_env->prefix.lock = true; break;
			case 0xf2: emu_env->prefix.repne = true; break;
			case 0xf3: emu_env->prefix.repe = true; break;
			default: read_prefix = false; break;
		}
		if (read_prefix) {
			emu_env->pinst++;
			emu_env->pinst_len--;
		}
	} while (read_prefix);

	/* REX */
	if (emu_env->g64 && (emu_env->pinst[0] & 0xf0) == 0x40) {
		emu_env->prefix.rex.raw = emu_env->pinst[0];
		emu_env->pinst++;
		emu_env->pinst_len--;
		HALT_ON_ERRORCOND(emu_env->pinst_len > 0);
	}
}

/* Parse parts of an instruction after the opcode */
static void parse_postfix(emu_env_t * emu_env, bool has_modrm, bool has_sib,
						  size_t displacement_len, size_t immediate_len)
{

#define SET_DISP(x) \
	do { \
		HALT_ON_ERRORCOND(displacement_len == 0); \
		displacement_len = (x); \
	} while (0)

#define SET_SIB(x) \
	do { \
		HALT_ON_ERRORCOND(!has_sib); \
		has_sib = (x); \
	} while (0)

	if (has_modrm) {
		HALT_ON_ERRORCOND(emu_env->pinst_len >= 1);
		emu_env->postfix.modrm.raw = emu_env->pinst[0];
		emu_env->pinst++;
		emu_env->pinst_len--;

		/* Compute displacement */
		switch (emu_env->postfix.modrm.mod) {
		case 0:
			switch (get_address_size(emu_env)) {
			case BIT_SIZE_16:
				if (get_modrm_rm(emu_env) == 6) {
					SET_DISP(BIT_SIZE_16);
				}
				break;
			case BIT_SIZE_32:	/* fallthrough */
			case BIT_SIZE_64:
				if (get_modrm_rm(emu_env) % 8 == CPU_REG_BP) {
					SET_DISP(BIT_SIZE_32);
				}
				break;
			default:
				HALT_ON_ERRORCOND(0 && "Invalid value");
			}
			break;
		case 1:
			SET_DISP(BIT_SIZE_8);
			break;
		case 2:
			SET_DISP(MIN(get_address_size(emu_env), BIT_SIZE_32));
			break;
		case 3:
			break;
		default:
			HALT_ON_ERRORCOND(0 && "Invalid value");
		}

		/* Compute whether SIB is present */
		if (emu_env->postfix.modrm.mod != 3 &&
			get_address_size(emu_env) != BIT_SIZE_16 &&
			get_modrm_rm(emu_env) % 8 == CPU_REG_SP) {
			SET_SIB(true);
		}
	}
	if (has_sib) {
		HALT_ON_ERRORCOND(emu_env->pinst_len >= 1);
		emu_env->postfix.sib.raw = emu_env->pinst[0];
		emu_env->pinst++;
		emu_env->pinst_len--;

		/* Compute displacement if mod=0, base=5 */
		if (emu_env->postfix.modrm.mod == 0 &&
			get_sib_base(emu_env) % 8 == CPU_REG_BP) {
			SET_DISP(BIT_SIZE_32);
		}
	}
	if (displacement_len > 0) {
		HALT_ON_ERRORCOND(emu_env->pinst_len >= displacement_len);
		emu_env->postfix.displacement = emu_env->pinst;
		emu_env->pinst += displacement_len;
		emu_env->pinst_len -= displacement_len;
	}
	if (immediate_len > 0) {
		HALT_ON_ERRORCOND(emu_env->pinst_len >= immediate_len);
		emu_env->postfix.immediate = emu_env->pinst;
		emu_env->pinst += immediate_len;
		emu_env->pinst_len -= immediate_len;
	}
	HALT_ON_ERRORCOND(emu_env->pinst_len == 0);

	emu_env->displacement_len = displacement_len;
	emu_env->immediate_len = immediate_len;

#undef SET_DISP
#undef SET_SIB

}

/* Parse first byte of opcode */
static void parse_opcode_one(emu_env_t * emu_env)
{
	u8 opcode;
	HALT_ON_ERRORCOND(emu_env->pinst_len > 0);
	opcode = emu_env->pinst[0];
	emu_env->pinst++;
	emu_env->pinst_len--;
	switch (opcode) {
	case 0x00: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x01: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x02: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x03: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x04: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x05: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x06: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x07: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x08: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x09: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x0f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x10: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x11: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x12: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x13: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x14: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x15: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x16: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x17: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x18: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x19: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x1f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x20: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x21: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x22: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x23: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x24: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x25: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x26: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x27: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x28: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x29: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x2a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x2b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x2c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x2d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x2e: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x2f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x30: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x31: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x32: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x33: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x34: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x35: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x36: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x37: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x38: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x39: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x3a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x3b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x3c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x3d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x3e: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x3f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x40: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x41: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x42: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x43: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x44: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x45: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x46: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x47: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x48: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x49: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x4f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x50: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x51: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x52: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x53: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x54: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x55: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x56: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x57: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x58: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x59: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x5f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x60: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x61: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x62: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x63: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x64: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x65: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x66: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x67: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0x68: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x69: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x6f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x70: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x71: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x72: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x73: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x74: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x75: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x76: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x77: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x78: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x79: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x7f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x80: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x81: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x82: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x83: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x84: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x85: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x86: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x87: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x88: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x89:	/* MOV Ev, Gv */
		HALT_ON_ERRORCOND(!emu_env->prefix.lock && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repe && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repne && "Not implemented");
		parse_postfix(emu_env, true, false, 0, 0);
		{
			size_t operand_size = get_operand_size(emu_env);
			uintptr_t rm;	/* w */
			void *reg = eval_modrm_reg(emu_env);	/* r */
			if (eval_modrm_addr(emu_env, &rm)) {
				mem_access_env_t env = {
					.haddr = reg,
					.gaddr = rm,
					.seg = emu_env->seg,
					.size = operand_size,
					.mode = HPT_PROT_WRITE_MASK,
					.cpl = emu_env->vcpu->vmcs.guest_CS_selector & 3,
				};
				access_memory_gv(&emu_env->ctx_pair, &env);
			} else {
				memcpy((void *)rm, reg, operand_size);
			}
		}
		break;
	case 0x8a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x8b:	/* MOV Gv, Ev */
		HALT_ON_ERRORCOND(!emu_env->prefix.lock && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repe && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repne && "Not implemented");
		parse_postfix(emu_env, true, false, 0, 0);
		{
			size_t operand_size = get_operand_size(emu_env);
			uintptr_t rm;	/* r */
			void *reg = eval_modrm_reg(emu_env);	/* w */
			if (eval_modrm_addr(emu_env, &rm)) {
				mem_access_env_t env = {
					.haddr = reg,
					.gaddr = rm,
					.seg = emu_env->seg,
					.size = operand_size,
					.mode = HPT_PROT_READ_MASK,
					.cpl = emu_env->vcpu->vmcs.guest_CS_selector & 3,
				};
				access_memory_gv(&emu_env->ctx_pair, &env);
			} else {
				memcpy(reg, (void *)rm, operand_size);
			}
		}
		break;
	case 0x8c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x8d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x8e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x8f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x90: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x91: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x92: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x93: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x94: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x95: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x96: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x97: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x98: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x99: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9a: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9b: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9c: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9d: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9e: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0x9f: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa0: /* MOV AL, Ob */
	case 0xa1: /* MOV rAX, Ov */
	case 0xa2: /* MOV Ob, AL */
	case 0xa3: /* MOV Ov, rAX */
		HALT_ON_ERRORCOND(!emu_env->prefix.lock && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repe && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repne && "Not implemented");
		{
			size_t address_size = get_address_size(emu_env);
			size_t operand_size = (opcode & 1) ? get_operand_size(emu_env) : 1;
			mem_access_env_t env;
			parse_postfix(emu_env, false, false, 0, address_size);
			compute_segment(emu_env, CPU_REG_AX);
			env = (mem_access_env_t){
				.haddr = get_reg_ptr(emu_env, CPU_REG_AX, operand_size),
				.gaddr = 0,
				.seg = emu_env->seg,
				.size = operand_size,
				.mode = (opcode & 2) ? HPT_PROT_WRITE_MASK : HPT_PROT_READ_MASK,
				.cpl = emu_env->vcpu->vmcs.guest_CS_selector & 3,
			};
			zero_extend(&env.gaddr, emu_env->postfix.immediate,
						sizeof(env.gaddr), address_size);
			access_memory_gv(&emu_env->ctx_pair, &env);
		}
		break;
	case 0xa4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa7: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xa9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xaa: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xab: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xac: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xad: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xae: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xaf: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb0: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb1: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb2: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb3: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb7: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xb9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xba: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xbb: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xbc: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xbd: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xbe: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xbf: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc0: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc1: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc2: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc3: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc7:	/* MOV Ev, Iz */
		HALT_ON_ERRORCOND(!emu_env->prefix.lock && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repe && "Not implemented");
		HALT_ON_ERRORCOND(!emu_env->prefix.repne && "Not implemented");
		if (emu_env->postfix.modrm.regop == 0) {
			size_t operand_size = get_operand_size(emu_env);
			size_t imm_size = MIN(operand_size, BIT_SIZE_32);
			u64 imm;
			void *pimm;
			uintptr_t rm;	/* w */
			parse_postfix(emu_env, true, false, 0, imm_size);
			if (operand_size == BIT_SIZE_64) {
				imm = (int64_t)*(int32_t *)emu_env->postfix.immediate4;
				pimm = &imm;
			} else {
				pimm = emu_env->postfix.immediate;
			}
			if (eval_modrm_addr(emu_env, &rm)) {
				mem_access_env_t env = {
					.haddr = (void *)pimm,
					.gaddr = rm,
					.seg = emu_env->seg,
					.size = operand_size,
					.mode = HPT_PROT_WRITE_MASK,
					.cpl = emu_env->vcpu->vmcs.guest_CS_selector & 3,
				};
				access_memory_gv(&emu_env->ctx_pair, &env);
			} else {
				memcpy((void *)rm, (void *)pimm, operand_size);
			}
		} else {
			HALT_ON_ERRORCOND(0 && "Not implemented");
		}
		break;
	case 0xc8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xc9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xca: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xcb: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xcc: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xcd: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xce: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xcf: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd0: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd1: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd2: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd3: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd7: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xd9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xda: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xdb: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xdc: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xdd: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xde: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xdf: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe0: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe1: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe2: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe3: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe7: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xe9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xea: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xeb: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xec: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xed: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xee: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xef: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf0: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0xf1: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf2: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0xf3: HALT_ON_ERRORCOND(0 && "Prefix operand"); break;
	case 0xf4: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf5: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf6: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf7: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf8: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xf9: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xfa: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xfb: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xfc: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xfd: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xfe: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	case 0xff: HALT_ON_ERRORCOND(0 && "Not implemented"); break;
	default:
		HALT_ON_ERRORCOND(0 && "Not implemented");
	}
}

/*
 * Emulate instruction by changing the VMCS values.
 * Currently XMHF will crash if the instruction is invalid.
 */
void emulate_instruction(VCPU * vcpu, struct regs *r)
{
	emu_env_t emu_env = { .prefix=PREFIX_INFO_INITIALIZER };
	u8 inst[INST_LEN_MAX] = {};
	uintptr_t rip = vcpu->vmcs.guest_RIP;
	u32 inst_len = vcpu->vmcs.info_vmexit_instruction_length;

	emu_env.vcpu = vcpu;
	emu_env.r = r;
	guestmem_init(vcpu, &emu_env.ctx_pair);
	emu_env.g64 = VCPU_g64(vcpu);
	emu_env.cs_d = !!(vcpu->vmcs.guest_CS_access_rights & (1 << 14));
	emu_env.seg = CPU_SEG_UNKNOWN;

	/* Fetch instruction */
	HALT_ON_ERRORCOND(inst_len < INST_LEN_MAX);
	{
		mem_access_env_t env = {
			.haddr = inst,
			.gaddr = rip,
			.seg = CPU_SEG_CS,
			.size = inst_len,
			.mode = HPT_PROT_EXEC_MASK,
			.cpl = vcpu->vmcs.guest_CS_selector & 3,
		};
		access_memory_gv(&emu_env.ctx_pair, &env);
	}
	printf("CPU(0x%02x): emulation: %d 0x%llx\n", vcpu->id, inst_len,
		   *(u64 *)inst);

	/* Parse prefix and opcode */
	emu_env.pinst = inst;
	emu_env.pinst_len = inst_len;

	parse_prefix(&emu_env);
	parse_opcode_one(&emu_env);

	// TODO: Should not increase RIP if string instrcution
	HALT_ON_ERRORCOND(!emu_env.prefix.repe && !emu_env.prefix.repne);
	vcpu->vmcs.guest_RIP += inst_len;
}
