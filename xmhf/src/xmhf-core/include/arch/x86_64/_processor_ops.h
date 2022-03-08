#ifndef __PROCESSOR_OPS_H
#define __PROCESSOR_OPS_H

#ifndef __ASSEMBLY__
#include "_processor.h"
#include "_error.h"

// Selectors for <CPU_Reg_Read> and <CPU_Reg_Write>
enum CPU_Reg_Sel 
{ 
    CPU_REG_AX,
    CPU_REG_BX,
    CPU_REG_CX,
    CPU_REG_DX,
    CPU_REG_SI,
    CPU_REG_DI,
    CPU_REG_SP,
    CPU_REG_BP,

    CPU_REG_r8,
    CPU_REG_r9,
    CPU_REG_r10,
    CPU_REG_r11,
    CPU_REG_r12,
    CPU_REG_r13,
    CPU_REG_r14,
    CPU_REG_r15
};

static inline ulong_t CPU_Reg_Read(struct regs* r, enum CPU_Reg_Sel sel)
{
    switch (sel)
    {
        case CPU_REG_AX:
            return r->rax;
        case CPU_REG_BX:
            return r->rbx;
        case CPU_REG_CX:
            return r->rcx;
        case CPU_REG_DX:
            return r->rdx;
        case CPU_REG_SI:
            return r->rsi;
        case CPU_REG_DI:
            return r->rdi;
        case CPU_REG_SP:
            return r->rsp;
        case CPU_REG_BP:
            return r->rbp;

        case CPU_REG_r8:
            return r->r8;
        case CPU_REG_r9:
            return r->r9;
        case CPU_REG_r10:
            return r->r10;
        case CPU_REG_r11:
            return r->r11;
        case CPU_REG_r12:
            return r->r12;
        case CPU_REG_r13:
            return r->r13;
        case CPU_REG_r14:
            return r->r14;
        case CPU_REG_r15:
            return r->r15;

        default:
            printf("CPU_Reg_Read: Invalid CPU register is given (sel:%u)!\n", sel);
            HALT_ON_ERRORCOND(false);
            return 0; // should never hit
    }
}

#endif // __ASSEMBLY__
#endif // __PROCESSOR_OPS_H