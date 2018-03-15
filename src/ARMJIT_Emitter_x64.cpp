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

/*
  register mapping

  eax
  ecx
  edx
  ebx
  esp
  ebp
  esi
  edi

  r8d
  r9d
  ...
  r14d
  r15d

  r0 -> ebx
  r1 -> ebp
  r2 -> edi
  r3 -> esi
  r4 -> r12d
  r5 -> r13d
  r6 -> r14d
  r7 -> r15d
  r8 -> ecx
  r9 -> edx
  r10 -> r8d
  r11 -> r9d
  r12 -> r10d
  r13 -> r11d
  r14 -> mem. storage (LR, seldom used as working register)
  r15 -> mem. storage (only matters at compile time)

  eax -> JIT working register

  calling conventions:

  when beginning/ending a JIT block:
  save x86 registers: ebx, ebp, r12d, r13d, r14d, r15d (plus: edi, esi under Windows)
  load ARM registers from CPU data structure
  (run compiled code)
  save ARM registers to CPU data structure
  restore x86 registers

  when calling an external function:
  save x86 registers: ecx, edx, r8d, r9d, r10d, r11d (plus: edi, esi under Linux) (plus: eax if needed)
  set up function arguments:
    Windows: ecx, edx, r8d, r9d
    Linux: edi, esi, edx, ecx, r8d, r9d
  call function
  restore x86 registers (sans eax)
  use return value in eax
  restore eax if needed
*/

#include "ARMJIT.h"
#include "ARMJIT_Emitter.h"


namespace ARMJIT
{

namespace Emitter
{

enum X64Regs
{
    EAX = 0,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI,

    R8D = 0x80,
    R9D,
    R10D,
    R11D,
    R12D,
    R13D,
    R14D,
    R15D
};

u8 RegMapping[16] =
{
    EBX, EBP, ESI, EDI, R12D, R13D, R14D, R15D,
    ECX, EDX, R8D, R9D, R10D, R11D, 0xFF, 0xFF
};

u8* CodeBuffer;
u8* C;


void SetCodeBuffer(u8* buf)
{
    CodeBuffer = buf;
    C = buf;
}


// technically PUSH/POP act on whole 64bit registers

void PUSH(u8 reg)
{
    if (reg & 0x80) *C++ = 0x41;
    *C++ = 0x50 + (reg & 0x07);
}

void POP(u8 reg)
{
    if (reg & 0x80) *C++ = 0x41;
    *C++ = 0x58 + (reg & 0x07);
}

void MOV_Imm32_Mem(u32 imm, void* ptr)
{
    *C++ = 0xC7;
    *C++ = 0x04;
    *C++ = 0x25;
    *(u32*)C = (u32)(((u64)ptr) & 0xFFFFFFFF);
    C += 4;
    *(u32*)C = imm;
    C += 4;
}


}

}
