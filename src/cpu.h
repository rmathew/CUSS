// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CPU_INCLUDED
#define CUSS_CPU_INCLUDED

#include <stdbool.h>

#include "errors.h"

extern void CussInitCpu(void);

extern bool CussExecNextInsn(CuError* restrict err);

extern bool CussPrintCpuState(CuError* restrict err);

#endif  // CUSS_CPU_INCLUDED
