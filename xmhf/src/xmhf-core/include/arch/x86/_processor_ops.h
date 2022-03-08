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
};

static inline ulong_t CPU_Reg_Read(struct regs* r, enum CPU_Reg_Sel sel)
{
    switch (sel)
    {
        case CPU_REG_AX:
            return r->eax;
        case CPU_REG_BX:
            return r->ebx;
        case CPU_REG_CX:
            return r->ecx;
        case CPU_REG_DX:
            return r->edx;
        case CPU_REG_SI:
            return r->esi;
        case CPU_REG_DI:
            return r->edi;
        case CPU_REG_SP:
            return r->esp;
        case CPU_REG_BP:
            return r->ebp;

        default:
            printf("CPU_Reg_Read: Invalid CPU register is given (sel:%u)!\n", sel);
            HALT_ON_ERRORCOND(false);
            return 0; // should never hit
    }
}

#endif // __ASSEMBLY__
#endif // __PROCESSOR_OPS_H