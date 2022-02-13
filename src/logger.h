// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_LOGGER_INCLUDED
#define CUSS_LOGGER_INCLUDED

extern void CuLogError(const char* restrict fmt, ...);
extern void CuLogWarn(const char* restrict fmt, ...);
extern void CuLogInfo(const char* restrict fmt, ...);

#endif  // CUSS_LOGGER_INCLUDED
