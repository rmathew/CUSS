// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CPU_INCLUDED
#define CUSS_CPU_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern void CuInitCpu(void);

extern bool CuGetIntReg(uint8_t r_n, uint32_t* restrict r_val,
  CuError* restrict err);
extern bool CuSetIntReg(uint8_t r_n, uint32_t r_val, CuError* restrict err);

extern uint32_t CuGetExtPrecReg();
extern void CuSetExtPrecReg(uint32_t r_val);

extern uint32_t CuGetProgCtr(void);
extern bool CuSetProgCtr(uint32_t pc, CuError* restrict err);

extern bool CuIsNegFlagSet(void);
extern bool CuIsOvfFlagSet(void);
extern bool CuIsCarFlagSet(void);
extern bool CuIsZerFlagSet(void);
extern void CuSetIntFlags(bool neg, bool ovf, bool car, bool zer);

extern bool CuExecNextInsn(CuError* restrict err);

extern bool CuPrintCpuState(CuError* restrict err);

#endif  // CUSS_CPU_INCLUDED
