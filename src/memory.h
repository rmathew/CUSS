// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_MEMORY_INCLUDED
#define CUSS_MEMORY_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern bool CuIsValidPhyMemAddr(uint32_t addr, CuError* restrict err);

extern bool CuByteAt(uint32_t addr, uint8_t* restrict val,
  CuError* restrict err);

extern bool CuWordAt(uint32_t addr, uint32_t* restrict val,
  CuError* restrict err);

extern bool CuInitMemFromFile(const char* restrict file, CuError* restrict err);

#endif  // CUSS_MEMORY_INCLUDED
