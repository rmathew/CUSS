// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CPU_INCLUDED
#define CUSS_CPU_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern void CussInitCpu(void);

extern bool CussGetIntRegister(uint8_t r_n, uint32_t* restrict r_val,
  CuError* restrict err);

extern bool CussSetIntRegister(uint8_t r_n, uint32_t r_val,
  CuError* restrict err);

extern uint32_t CussGetProgramCounter(void);
extern bool CussSetProgramCounter(uint32_t pc, CuError* restrict err);

extern bool CussExecNextInsn(CuError* restrict err);

extern bool CussPrintCpuState(CuError* restrict err);

#endif  // CUSS_CPU_INCLUDED
