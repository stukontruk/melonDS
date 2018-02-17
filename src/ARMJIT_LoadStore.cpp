/*
    Copyright 2016-2017 StapleButter

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include "ARM.h"


namespace ARMJIT
{


// copypasta from ALU. bad
#define LSL_IMM(x, s) \
    x <<= s;

#define LSR_IMM(x, s) \
    if (s == 0) x = 0; \
    else        x >>= s;

#define ASR_IMM(x, s) \
    if (s == 0) x = ((s32)x) >> 31; \
    else        x = ((s32)x) >> s;

#define ROR_IMM(x, s) \
    if (s == 0) \
    { \
        x = (x >> 1) | ((cpu->CPSR & 0x20000000) << 2); \
    } \
    else \
    { \
        x = ROR(x, s); \
    }



#define A_WB_CALC_OFFSET_IMM \
    //u32 offset = (cpu->CurInstr & 0xFFF); \
    if (!(cpu->CurInstr & (1<<23))) offset = -offset;

#define A_WB_CALC_OFFSET_REG(shiftop) \
    //u32 offset = cpu->R[cpu->CurInstr & 0xF]; \
    u32 shift = ((cpu->CurInstr>>7)&0x1F); \
    shiftop(offset, shift); \
    if (!(cpu->CurInstr & (1<<23))) offset = -offset;



#define A_STR \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite32(offset, cpu->R[(cpu->CurInstr>>12) & 0xF]); \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset;

#define A_STR_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite32(addr, cpu->R[(cpu->CurInstr>>12) & 0xF], cpu->CurInstr & (1<<21)); \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset;

#define A_STRB \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite8(offset, cpu->R[(cpu->CurInstr>>12) & 0xF]); \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset;

#define A_STRB_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite8(addr, cpu->R[(cpu->CurInstr>>12) & 0xF], cpu->CurInstr & (1<<21)); \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset;

#define A_LDR \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    u32 val = cpu->DataRead32(offset); val = ROR(val, ((offset&0x3)<<3)); \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->Cycles += 1; \
    if (((cpu->CurInstr>>12) & 0xF) == 15) \
    { \
        if (cpu->Num==1) val &= ~0x1; \
        cpu->JumpTo(val); \
    } \
    else \
    { \
        cpu->R[(cpu->CurInstr>>12) & 0xF] = val; \
    }

#define A_LDR_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    u32 val = cpu->DataRead32(addr, cpu->CurInstr & (1<<21)); val = ROR(val, ((addr&0x3)<<3)); \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->Cycles += 1; \
    if (((cpu->CurInstr>>12) & 0xF) == 15) \
    { \
        if (cpu->Num==1) val &= ~0x1; \
        cpu->JumpTo(val); \
    } \
    else \
    { \
        cpu->R[(cpu->CurInstr>>12) & 0xF] = val; \
    }

#define A_LDRB \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    u32 val = cpu->DataRead8(offset); \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->Cycles += 1; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = val; \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRB PC %08X\n", cpu->R[15]); \

#define A_LDRB_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    u32 val = cpu->DataRead8(addr, cpu->CurInstr & (1<<21)); \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->Cycles += 1; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = val; \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRB PC %08X\n", cpu->R[15]); \



#define A_IMPLEMENT_WB_LDRSTR(x) \
\
void A_##x##_IMM(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_IMM \
    A_##x \
} \
\
void A_##x##_REG_LSL(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(LSL_IMM) \
    A_##x \
} \
\
void A_##x##_REG_LSR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(LSR_IMM) \
    A_##x \
} \
\
void A_##x##_REG_ASR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(ASR_IMM) \
    A_##x \
} \
\
void A_##x##_REG_ROR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(ROR_IMM) \
    A_##x \
} \
\
void A_##x##_POST_IMM(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_IMM \
    A_##x##_POST \
} \
\
void A_##x##_POST_REG_LSL(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(LSL_IMM) \
    A_##x##_POST \
} \
\
void A_##x##_POST_REG_LSR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(LSR_IMM) \
    A_##x##_POST \
} \
\
void A_##x##_POST_REG_ASR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(ASR_IMM) \
    A_##x##_POST \
} \
\
void A_##x##_POST_REG_ROR(ARM* cpu) \
{ \
    A_WB_CALC_OFFSET_REG(ROR_IMM) \
    A_##x##_POST \
}

A_IMPLEMENT_WB_LDRSTR(STR)
A_IMPLEMENT_WB_LDRSTR(STRB)
A_IMPLEMENT_WB_LDRSTR(LDR)
A_IMPLEMENT_WB_LDRSTR(LDRB)



#define A_HD_CALC_OFFSET_IMM \
    //u32 offset = (cpu->CurInstr & 0xF) | ((cpu->CurInstr >> 4) & 0xF0); \
    if (!(cpu->CurInstr & (1<<23))) offset = -offset;

#define A_HD_CALC_OFFSET_REG \
    //u32 offset = cpu->R[cpu->CurInstr & 0xF]; \
    if (!(cpu->CurInstr & (1<<23))) offset = -offset;



#define A_STRH \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite16(offset, cpu->R[(cpu->CurInstr>>12) & 0xF]); \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \

#define A_STRH_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->DataWrite16(addr, cpu->R[(cpu->CurInstr>>12) & 0xF]); \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \

// TODO: CHECK LDRD/STRD TIMINGS!!

#define A_LDRD \
    if (cpu->Num != 0) return; \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->Cycles += 1; \
    u32 r = (cpu->CurInstr>>12) & 0xF; \
    cpu->R[r  ] = cpu->DataRead32(offset  ); \
    cpu->R[r+1] = cpu->DataRead32(offset+4); \

#define A_LDRD_POST \
    if (cpu->Num != 0) return; \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->Cycles += 1; \
    u32 r = (cpu->CurInstr>>12) & 0xF; \
    cpu->R[r  ] = cpu->DataRead32(addr  ); \
    cpu->R[r+1] = cpu->DataRead32(addr+4); \

#define A_STRD \
    if (cpu->Num != 0) return; \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    u32 r = (cpu->CurInstr>>12) & 0xF; \
    cpu->DataWrite32(offset  , cpu->R[r  ]); \
    cpu->DataWrite32(offset+4, cpu->R[r+1]); \

#define A_STRD_POST \
    if (cpu->Num != 0) return; \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    u32 r = (cpu->CurInstr>>12) & 0xF; \
    cpu->DataWrite32(offset  , cpu->R[r  ]); \
    cpu->DataWrite32(offset+4, cpu->R[r+1]); \

#define A_LDRH \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = cpu->DataRead16(offset); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRH PC %08X\n", cpu->R[15]); \

#define A_LDRH_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = cpu->DataRead16(addr); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRH PC %08X\n", cpu->R[15]); \

#define A_LDRSB \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = (s32)(s8)cpu->DataRead8(offset); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRSB PC %08X\n", cpu->R[15]); \

#define A_LDRSB_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = (s32)(s8)cpu->DataRead8(addr); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRSB PC %08X\n", cpu->R[15]); \

#define A_LDRSH \
    offset += cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    if (cpu->CurInstr & (1<<21)) cpu->R[(cpu->CurInstr>>16) & 0xF] = offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = (s32)(s16)cpu->DataRead16(offset); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRSH PC %08X\n", cpu->R[15]); \

#define A_LDRSH_POST \
    u32 addr = cpu->R[(cpu->CurInstr>>16) & 0xF]; \
    cpu->R[(cpu->CurInstr>>16) & 0xF] += offset; \
    cpu->R[(cpu->CurInstr>>12) & 0xF] = (s32)(s16)cpu->DataRead16(addr); \
    if (((cpu->CurInstr>>12) & 0xF) == 15) printf("!! LDRSH PC %08X\n", cpu->R[15]); \


#define A_IMPLEMENT_HD_LDRSTR(x) \
\
void A_##x##_IMM(ARM* cpu) \
{ \
    A_HD_CALC_OFFSET_IMM \
    A_##x \
} \
\
void A_##x##_REG(ARM* cpu) \
{ \
    A_HD_CALC_OFFSET_REG \
    A_##x \
} \
void A_##x##_POST_IMM(ARM* cpu) \
{ \
    A_HD_CALC_OFFSET_IMM \
    A_##x##_POST \
} \
\
void A_##x##_POST_REG(ARM* cpu) \
{ \
    A_HD_CALC_OFFSET_REG \
    A_##x##_POST \
}

A_IMPLEMENT_HD_LDRSTR(STRH)
A_IMPLEMENT_HD_LDRSTR(LDRD)
A_IMPLEMENT_HD_LDRSTR(STRD)
A_IMPLEMENT_HD_LDRSTR(LDRH)
A_IMPLEMENT_HD_LDRSTR(LDRSB)
A_IMPLEMENT_HD_LDRSTR(LDRSH)



void A_SWP(ARM* cpu)
{

}

void A_SWPB(ARM* cpu)
{

}



void A_LDM(ARM* cpu)
{

}

void A_STM(ARM* cpu)
{

}




// ---- THUMB -----------------------



void T_LDR_PCREL(ARM* cpu)
{

}


void T_STR_REG(ARM* cpu)
{

}

void T_STRB_REG(ARM* cpu)
{

}

void T_LDR_REG(ARM* cpu)
{

}

void T_LDRB_REG(ARM* cpu)
{

}


void T_STRH_REG(ARM* cpu)
{

}

void T_LDRSB_REG(ARM* cpu)
{

}

void T_LDRH_REG(ARM* cpu)
{

}

void T_LDRSH_REG(ARM* cpu)
{

}


void T_STR_IMM(ARM* cpu)
{

}

void T_LDR_IMM(ARM* cpu)
{

}

void T_STRB_IMM(ARM* cpu)
{

}

void T_LDRB_IMM(ARM* cpu)
{

}


void T_STRH_IMM(ARM* cpu)
{

}

void T_LDRH_IMM(ARM* cpu)
{

}


void T_STR_SPREL(ARM* cpu)
{

}

void T_LDR_SPREL(ARM* cpu)
{

}


void T_PUSH(ARM* cpu)
{

}

void T_POP(ARM* cpu)
{

}

void T_STMIA(ARM* cpu)
{

}

void T_LDMIA(ARM* cpu)
{

}


}

