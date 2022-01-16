// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_OPS_INCLUDED
#define CUSS_OPS_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern void CussInitOps(void);

extern bool CussExecuteOp(uint32_t pc, uint32_t insn, CuError* restrict err);

#endif  // CUSS_OPS_INCLUDED
