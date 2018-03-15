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

#ifdef __WIN32__
    #include <windows.h>
#else
    // TODO!!
#endif

#include <stdio.h>
#include "NDS.h"
#include "CP15.h"
#include "ARMJIT.h"
#include "ARMJIT_ALU.h"
#include "ARMJIT_Branch.h"
#include "ARMJIT_LoadStore.h"


namespace ARMJIT
{


const int kBlockSize = 32*1024;
const int kNumBlocks = 256;
const int kMaxBlocksPerPage = 4;

u8* CodeCache[2];
u32* CodeCacheIndex[2];            // memory addr tag -> code block number
u32* CodeCacheReverseIndex[2];     // block number -> memory address
u32 NextCodeBlockID[2];


bool Init()
{
#ifdef __WIN32__
    CodeCache[0] = (u8*)VirtualAlloc(NULL, kNumBlocks*kBlockSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    CodeCache[1] = (u8*)VirtualAlloc(NULL, kNumBlocks*kBlockSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    // TODO!!
    CodeCache[0] = NULL;
    CodeCache[1] = NULL;
#endif

    if (!CodeCache[0] || !CodeCache[1])
    {
        printf("JIT: code cache allocation failed\n");
        return false;
    }

    // 0x0_XXXX00
    CodeCacheIndex[0] = new u32[65536*kMaxBlocksPerPage];
    CodeCacheIndex[1] = new u32[65536*kMaxBlocksPerPage];
    CodeCacheReverseIndex[0] = new u32[kNumBlocks];
    CodeCacheReverseIndex[1] = new u32[kNumBlocks];

    return true;
}

void DeInit()
{
    if (CodeCache[0] && CodeCache[1])
    {
#ifdef __WIN32__
       VirtualFree(CodeCache[0], 0, MEM_RELEASE);
       VirtualFree(CodeCache[1], 0, MEM_RELEASE);
#else
        // TODO!!
        CodeCache[0] = NULL;
        CodeCache[1] = NULL;
#endif
    }

    delete[] CodeCacheIndex[0];
    delete[] CodeCacheIndex[1];
    delete[] CodeCacheReverseIndex[0];
    delete[] CodeCacheReverseIndex[1];
}

void Reset()
{
    if (CodeCache[0] && CodeCache[1])
    {
        memset(CodeCache[0], 0xCC, kNumBlocks*kBlockSize);
        memset(CodeCache[1], 0xCC, kNumBlocks*kBlockSize);
    }

    memset(CodeCacheIndex[0], 0xFF, 65536*kMaxBlocksPerPage*sizeof(u32));
    memset(CodeCacheIndex[1], 0xFF, 65536*kMaxBlocksPerPage*sizeof(u32));
    memset(CodeCacheReverseIndex[0], 0xFF, kNumBlocks*sizeof(u32));
    memset(CodeCacheReverseIndex[1], 0xFF, kNumBlocks*sizeof(u32));

    NextCodeBlockID[0] = 0;
    NextCodeBlockID[1] = 0;
}


void InvalidateCache(u32 cpu, u32 addr)
{
    u32* candidates = &CodeCacheIndex[cpu][(addr >> 8) & 0xFFFF];
    for (int i = 0; i < kMaxBlocksPerPage; i++)
    {
        u32 id = candidates[i];

        if (id == 0xFFFFFFFF)
            continue;

        if (CodeCacheReverseIndex[cpu][id] == addr)
        {
            candidates[i] = 0xFFFFFFFF;
            CodeCacheReverseIndex[cpu][id] = 0xFFFFFFFF;
        }
    }
}

u32 CompileCode(ARM* cpu, u32 addr)
{
    u32 id = NextCodeBlockID[cpu->Num];
    u32 addr_tag = (addr >> 8) & 0xFFFF;

    u32 oldaddr = CodeCacheReverseIndex[cpu->Num][id];
    if (oldaddr != 0xFFFFFFFF)
    {
        // evict it
        InvalidateCache(cpu->Num, addr);
    }

    // compile new code
    // .. sorry
    // TODO actual code!!
    u8* code = &CodeCache[cpu->Num][id * kBlockSize];
    // c7 04 25 78 56 00 00 44 44 34 12    mov    DWORD PTR ds:0x5678,0x12344444
    *code++ = 0xC7;
    *code++ = 0x04;
    *code++ = 0x25;
    *(u32*)code = (u32)(((u64)&cpu->Cycles) & 0xFFFFFFFF);
    code += 4;
    *(u32*)code = cpu->Num?0x77777777:0x99999999;
    code += 4;
    *code = 0xC3; // RET

    u32* candidates = &CodeCacheIndex[cpu->Num][(addr >> 8) & 0xFFFF];
    bool good = false;
    for (int i = 0; i < kMaxBlocksPerPage; i++)
    {
        if (candidates[i] == 0xFFFFFFFF)
        {
            candidates[i] = id;
            good = true;
            break;
        }
    }

    if (!good)
    {
        printf("!!!! CACHE INDEX FULL FOR TAG %04X (ADDR %08X)\n", (addr>>8)&0xFFFF, addr);
        return -1;
    }

    CodeCacheReverseIndex[cpu->Num][id] = addr;

    NextCodeBlockID[cpu->Num]++;
    if (NextCodeBlockID[cpu->Num] >= kNumBlocks)
        NextCodeBlockID[cpu->Num] = 0;

    return id;
}

CodeBlock LookupCode(ARM* cpu)
{
    u32 addr = cpu->R[15] - ((cpu->CPSR & 0x20) ? 2 : 4);

    u32* candidates = &CodeCacheIndex[cpu->Num][(addr >> 8) & 0xFFFF];
    u32 blockid = -1;
    for (int i = 0; i < kMaxBlocksPerPage; i++)
    {
        u32 id = candidates[i];

        if (id == 0xFFFFFFFF)
            continue;

        if (CodeCacheReverseIndex[cpu->Num][id] == addr)
        {
            blockid = id;
            break;
        }
    }

    if (blockid == -1)
    {
        // compile it!
        printf("block for ARM%c %08X not found, compiling it\n", cpu->Num?'7':'9', addr);

        blockid = CompileCode(cpu, addr);
    }

    u8* code = &CodeCache[cpu->Num][blockid * kBlockSize];
    return (CodeBlock)code;
}


void A_UNK(ARM* cpu, u32 pc, u32 instr)
{
    printf("undefined ARM%d instruction %08X @ %08X\n", cpu->Num?7:9, instr, pc-8);
    //for (int i = 0; i < 16; i++) printf("R%d: %08X\n", i, cpu->R[i]);

}

void T_UNK(ARM* cpu, u32 pc, u32 instr)
{
    printf("undefined THUMB%d instruction %04X @ %08X\n", cpu->Num?7:9, instr, pc-4);

}



void A_MSR_IMM(ARM* cpu, u32 pc, u32 instr)
{

}

void A_MSR_REG(ARM* cpu, u32 pc, u32 instr)
{

}

void A_MRS(ARM* cpu, u32 pc, u32 instr)
{

}


void A_MCR(ARM* cpu, u32 pc, u32 instr)
{

}

void A_MRC(ARM* cpu, u32 pc, u32 instr)
{

}



void A_SVC(ARM* cpu, u32 pc, u32 instr)
{

}

void T_SVC(ARM* cpu, u32 pc, u32 instr)
{

}



#define INSTRFUNC_PROTO(x)  void (*x)(ARM* cpu, u32 pc, u32 instr)
#include "ARM_InstrTable.h"
#undef INSTRFUNC_PROTO

}
