// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "errors.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

bool CuErrMsg(CuError* restrict err, const char* restrict fmt, ...) {
    if (err == NULL || fmt == NULL) {
        return false;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err->err_msg, MAX_ERR_MSG_SIZE, fmt, ap);
    va_end(ap);
    return false;
}
