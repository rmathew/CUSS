// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CPU_INCLUDED
#define CUSS_CPU_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

// Number of integer registers.
#define CU_NUM_IREGS (1 << 5)

typedef enum {
    CU_CPU_ERROR = 0,
    CU_CPU_PAUSED,
    CU_CPU_RUNNING,
    CU_CPU_BREAK_POINT,
} CuCpuState;

extern void CuInitCpu(void);
extern CuCpuState CuGetCpuState(void);

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

extern bool CuContinueExec(CuError* restrict err);
extern bool CuExecNextInsn(CuError* restrict err);

extern bool CuAddBreakPoint(uint32_t addr, CuError* restrict err);
extern bool CuRemoveBreakPoint(uint32_t addr, CuError* restrict err);

#endif  // CUSS_CPU_INCLUDED
