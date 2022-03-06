// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_SDLUI_INCLUDED
#define CUSS_SDLUI_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "errors.h"

extern bool CuSdlUiSetUp(CuError* restrict err);
extern bool CuSdlUiTearDown(CuError* restrict err);

extern bool CuSdlUiRunEventLoop(CuError* restrict err);

#endif  // CUSS_SDLUI_INCLUDED
