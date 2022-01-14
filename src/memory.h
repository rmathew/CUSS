// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_MEMORY_INCLUDED
#define CUSS_MEMORY_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

#define CUSS_MEMSIZE (1 << 20)

extern uint8_t cuss_mem[];

extern bool CussInitMemFromFile(const char* restrict file,
  CuError* restrict err);

#endif  // CUSS_MEMORY_INCLUDED
