// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_MONITOR_INCLUDED
#define CUSS_MONITOR_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "errors.h"

// The type of a helper-function that lets the Monitor retrieve user-input.
typedef bool (*CuMonGetInpFn)(char* restrict buf, size_t buf_size,
  bool* restrict eof, CuError* restrict err);

// The type of a helper-function that lets the Monitor display user-output.
typedef bool (*CuMonPutMsgFn)(const char* restrict msg, CuError* restrict err);

extern bool CuMonSetUp(CuMonGetInpFn get_fn, CuMonPutMsgFn put_fn,
  CuError* restrict err);

extern bool CuRunMon(bool* restrict quit, CuError* restrict err);

#endif  // CUSS_MONITOR_INCLUDED
