// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "logger.h"

#include <stdarg.h>
#include <stdbool.h>

#include "SDL_log.h"

#define CU_SDL_LOG_MSG(PRI) \
  do { \
      if (fmt == NULL) { \
          return; \
      } \
      va_list ap; \
      va_start(ap, fmt); \
      SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, (PRI), fmt, ap); \
      va_end(ap); \
  } while (false)

void CuLogError(const char* restrict fmt, ...) {
    CU_SDL_LOG_MSG(SDL_LOG_PRIORITY_ERROR);
}

void CuLogWarn(const char* restrict fmt, ...) {
    CU_SDL_LOG_MSG(SDL_LOG_PRIORITY_WARN);
}

void CuLogInfo(const char* restrict fmt, ...) {
    CU_SDL_LOG_MSG(SDL_LOG_PRIORITY_INFO);
}
