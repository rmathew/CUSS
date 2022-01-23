// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_OPDEC_INCLUDED
#define CUSS_OPDEC_INCLUDED

#include <stddef.h>
#include <stdint.h>

extern void CuDecodeOp(uint32_t insn, char* restrict buf, size_t size);

#endif  // CUSS_OPDEC_INCLUDED
