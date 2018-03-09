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

#ifndef ARMJIT_H
#define ARMJIT_H

#include "types.h"
#include "ARM.h"

namespace ARMJIT
{

typedef void (*CodeBlock)();

bool Init();
void DeInit();
void Reset();

CodeBlock LookupCode(ARM* cpu);

extern void (*ARMInstrTable[4096])(ARM* cpu, u32 pc, u32 instr);
extern void (*THUMBInstrTable[1024])(ARM* cpu, u32 pc, u32 instr);

void A_BLX_IMM(ARM* cpu, u32 pc, u32 instr); // I'm a special one look at me

}

#endif // ARMJIT_H
