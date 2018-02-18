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
#include "NDS.h"
#include "CP15.h"
#include "ARMInterpreter.h"
#include "ARMInterpreter_ALU.h"
#include "ARMInterpreter_Branch.h"
#include "ARMInterpreter_LoadStore.h"


namespace ARMJIT
{


void A_UNK(ARM* cpu)
{
    printf("undefined ARM%d instruction %08X @ %08X\n", cpu->Num?7:9, cpu->CurInstr, cpu->R[15]-8);
    //for (int i = 0; i < 16; i++) printf("R%d: %08X\n", i, cpu->R[i]);

}

void T_UNK(ARM* cpu)
{
    printf("undefined THUMB%d instruction %04X @ %08X\n", cpu->Num?7:9, cpu->CurInstr, cpu->R[15]-4);

}



void A_MSR_IMM(ARM* cpu)
{

}

void A_MSR_REG(ARM* cpu)
{

}

void A_MRS(ARM* cpu)
{

}


void A_MCR(ARM* cpu)
{

}

void A_MRC(ARM* cpu)
{

}



void A_SVC(ARM* cpu)
{

}

void T_SVC(ARM* cpu)
{

}



#define INSTRFUNC_PROTO(x)  void (*x)(ARM* cpu)
#include "ARM_InstrTable.h"
#undef INSTRFUNC_PROTO

}
