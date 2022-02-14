// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CONCUR_INCLUDED
#define CUSS_CONCUR_INCLUDED

#include <stdbool.h>

#include "errors.h"

typedef struct CuThread* CuThread;

typedef int (*CuThreadFn)(void* data);

extern bool CuThrCreate(CuThreadFn fn, const char* restrict name,
  void* data, CuThread* restrict thr, CuError* restrict err);

extern void CuThrWait(CuThread* restrict thr, int* restrict status);

#endif  // CUSS_CONCUR_INCLUDED
