// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CPU_INCLUDED
#define CUSS_CPU_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern void CuInitCpu(void);

extern bool CuGetIntRegister(uint8_t r_n, uint32_t* restrict r_val,
  CuError* restrict err);
extern bool CuSetIntRegister(uint8_t r_n, uint32_t r_val,
  CuError* restrict err);

extern uint32_t CuGetProgramCounter(void);
extern bool CuSetProgramCounter(uint32_t pc, CuError* restrict err);

extern bool CuIsNegativeFlagSet(void);
extern bool CuIsOverflowFlagSet(void);
extern bool CuIsCarryFlagSet(void);
extern bool CuIsZeroFlagSet(void);
extern void CuSetIntegerFlags(bool negative, bool overflow, bool carry,
  bool zero);

extern bool CuExecNextInsn(CuError* restrict err);

extern bool CuPrintCpuState(CuError* restrict err);

#endif  // CUSS_CPU_INCLUDED
