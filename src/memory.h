// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_MEMORY_INCLUDED
#define CUSS_MEMORY_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern bool CuIsValidPhyMemAddr(uint32_t addr, CuError* restrict err);

extern bool CuGetByteAt(uint32_t addr, uint8_t* restrict val,
  CuError* restrict err);

extern bool CuGetHalfWordAt(uint32_t addr, uint16_t* restrict val,
  CuError* restrict err);

extern bool CuGetWordAt(uint32_t addr, uint32_t* restrict val,
  CuError* restrict err);

extern bool CuSetByteAt(uint32_t addr, uint8_t val, CuError* restrict err);
extern bool CuSetHalfWordAt(uint32_t addr, uint16_t val, CuError* restrict err);
extern bool CuSetWordAt(uint32_t addr, uint32_t val, CuError* restrict err);

extern bool CuInitMemFromFile(const char* restrict file, CuError* restrict err);

#endif  // CUSS_MEMORY_INCLUDED
