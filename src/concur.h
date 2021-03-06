// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_CONCUR_INCLUDED
#define CUSS_CONCUR_INCLUDED

#include <stdbool.h>

#include "errors.h"

typedef struct CuThread* CuThread;
typedef struct CuMutex* CuMutex;
typedef struct CuCondVar* CuCondVar;

typedef int (*CuThreadFn)(void* data);

extern bool CuThrCreate(CuThreadFn fn, const char* restrict name,
  void* restrict data, CuThread* restrict thr, CuError* restrict err);
extern bool CuThrWait(CuThread* restrict thr, int* restrict status,
  CuError* restrict err);

extern bool CuMutCreate(CuMutex* restrict mut, CuError* restrict err);
extern bool CuMutDestroy(CuMutex* restrict mut, CuError* restrict err);
extern bool CuMutLock(CuMutex* restrict mut, CuError* restrict err);
extern bool CuMutUnlock(CuMutex* restrict mut, CuError* restrict err);

extern bool CuCondVarCreate(CuCondVar* restrict cv, CuError* restrict err);
extern bool CuCondVarDestroy(CuCondVar* restrict cv, CuError* restrict err);
extern bool CuCondVarWait(CuCondVar* restrict cv, CuMutex* restrict mut,
  CuError* restrict err);
extern bool CuCondVarSignal(CuCondVar* restrict cv, CuError* restrict err);

#endif  // CUSS_CONCUR_INCLUDED
