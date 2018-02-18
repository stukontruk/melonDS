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

#ifndef ARM_H
#define ARM_H

#include "types.h"
#include "NDS.h"
#include "CP15.h"

// lame
#define C_S(x) x
#define C_N(x) x
#define C_I(x) x

#define ROR(x, n) (((x) >> (n)) | ((x) << (32-(n))))

class ARM
{
public:
    ARM(u32 num);
    ~ARM(); // destroy shit

    void Reset();

    void JumpTo(u32 addr, bool restorecpsr = false);
    void RestoreCPSR();

    void Halt(u32 halt)
    {
        if (halt==2 && Halted==1) return;
        Halted = halt;
    }

    void CheckIRQ()
    {
        if (!(NDS::IME[Num] & 0x1)) return;
        if (NDS::IF[Num] & NDS::IE[Num])
        {
            TriggerIRQ();
        }
    }

    s32 Execute();

    bool CheckCondition(u32 code)
    {
        if (code == 0xE) return true;
        if (ConditionTable[code] & (1 << (CPSR>>28))) return true;
        return false;
    }

    void SetC(bool c)
    {
        if (c) CPSR |= 0x20000000;
        else CPSR &= ~0x20000000;
    }

    void SetNZ(bool n, bool z)
    {
        CPSR &= ~0xC0000000;
        if (n) CPSR |= 0x80000000;
        if (z) CPSR |= 0x40000000;
    }

    void SetNZCV(bool n, bool z, bool c, bool v)
    {
        CPSR &= ~0xF0000000;
        if (n) CPSR |= 0x80000000;
        if (z) CPSR |= 0x40000000;
        if (c) CPSR |= 0x20000000;
        if (v) CPSR |= 0x10000000;
    }

    void UpdateMode(u32 oldmode, u32 newmode);

    void TriggerIRQ();


    u16 CodeRead16(u32 addr)
    {
        u16 val;
        // TODO eventually: on ARM9, THUMB opcodes are prefetched with 32bit reads
        if (!Num)
        {
            if (CP15::HandleCodeRead16(addr, &val))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                val = NDS::ARM9Read16(addr);
                Cycles += NDS::CodeAccessTimes[0][2][addr>>24];
            }
        }
        else
        {
            val = NDS::ARM7Read16(addr);
            Cycles += NDS::CodeAccessTimes[1][1][addr>>24];
        }

        return val;
    }

    u32 CodeRead32(u32 addr)
    {
        u32 val;
        if (!Num)
        {
            if (CP15::HandleCodeRead32(addr, &val))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                val = NDS::ARM9Read32(addr);
                Cycles += NDS::CodeAccessTimes[0][2][addr>>24] << 1;
            }
        }
        else
        {
            val = NDS::ARM7Read32(addr);
            Cycles += NDS::CodeAccessTimes[1][3][addr>>24];
        }

        return val;
    }


    u8 DataRead8(u32 addr, u32 forceuser=0)
    {
        u8 val;
        if (!Num)
        {
            if (CP15::HandleDataRead8(addr, &val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                val = NDS::ARM9Read8(addr);
                Cycles += NDS::DataAccessTimes[0][0][addr>>24] << 1;
            }
        }
        else
        {
            val = NDS::ARM7Read8(addr);
            Cycles += NDS::DataAccessTimes[1][0][addr>>24];
        }

        return val;
    }

    u16 DataRead16(u32 addr, u32 forceuser=0)
    {
        u16 val;
        addr &= ~1;
        if (!Num)
        {
            if (CP15::HandleDataRead16(addr, &val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                val = NDS::ARM9Read16(addr);
                Cycles += NDS::DataAccessTimes[0][0][addr>>24] << 1;
            }
        }
        else
        {
            val = NDS::ARM7Read16(addr);
            Cycles += NDS::DataAccessTimes[1][0][addr>>24];
        }

        return val;
    }

    u32 DataRead32(u32 addr, u32 forceuser=0)
    {
        u32 val;
        addr &= ~3;
        if (!Num)
        {
            if (CP15::HandleDataRead32(addr, &val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                val = NDS::ARM9Read32(addr);
                Cycles += NDS::DataAccessTimes[0][2][addr>>24] << 1;
            }
        }
        else
        {
            val = NDS::ARM7Read32(addr);
            Cycles += NDS::DataAccessTimes[1][2][addr>>24];
        }

        return val;
    }

    void DataWrite8(u32 addr, u8 val, u32 forceuser=0)
    {
        if (!Num)
        {
            if (CP15::HandleDataWrite8(addr, val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                NDS::ARM9Write8(addr, val);
                Cycles += NDS::DataAccessTimes[0][0][addr>>24] << 1;
            }
        }
        else
        {
            NDS::ARM7Write8(addr, val);
            Cycles += NDS::DataAccessTimes[1][0][addr>>24];
        }
    }

    void DataWrite16(u32 addr, u16 val, u32 forceuser=0)
    {
        addr &= ~1;
        if (!Num)
        {
            if (CP15::HandleDataWrite16(addr, val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                NDS::ARM9Write16(addr, val);
                Cycles += NDS::DataAccessTimes[0][0][addr>>24] << 1;
            }
        }
        else
        {
            NDS::ARM7Write16(addr, val);
            Cycles += NDS::DataAccessTimes[1][0][addr>>24];
        }
    }

    void DataWrite32(u32 addr, u32 val, u32 forceuser=0)
    {
        addr &= ~3;
        if (!Num)
        {
            if (CP15::HandleDataWrite32(addr, val, forceuser))
            {
                Cycles++; // cache/TCM access time
            }
            else
            {
                NDS::ARM9Write32(addr, val);
                Cycles += NDS::DataAccessTimes[0][2][addr>>24] << 1;
            }
        }
        else
        {
            NDS::ARM7Write32(addr, val);
            Cycles += NDS::DataAccessTimes[1][2][addr>>24];
        }
    }


    u32 Num;

    s32 Cycles;
    s32 CyclesToRun;
    u32 Halted;

    u32 R[16]; // heh
    u32 CPSR;
    u32 R_FIQ[8]; // holding SPSR too
    u32 R_SVC[3];
    u32 R_ABT[3];
    u32 R_IRQ[3];
    u32 R_UND[3];
    u32 CurInstr;
    u32 NextInstr[2];

    u32 ExceptionBase;

    static u32 ConditionTable[16];

    u32 debug;
};

// blarg

namespace ARMInterpreter
{

void A_UNK(ARM* cpu);
void T_UNK(ARM* cpu);

}

namespace ARMJIT
{

void A_UNK(ARM* cpu, u32 pc, u32 instr);
void T_UNK(ARM* cpu, u32 pc, u32 instr);

}

#endif // ARM_H
