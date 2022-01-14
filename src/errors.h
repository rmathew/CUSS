// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_ERRORS_INCLUDED
#define CUSS_ERRORS_INCLUDED

#include <stdbool.h>

#define MAX_ERR_MSG_SIZE (1 << 10)

typedef struct {
    char err_msg[MAX_ERR_MSG_SIZE];
} CuError;

extern bool CuErrMsg(CuError* restrict err, const char* restrict fmt, ...);

#endif  // CUSS_ERRORS_INCLUDED
