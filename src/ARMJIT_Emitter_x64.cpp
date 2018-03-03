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
