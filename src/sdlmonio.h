// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_SDLMONIO_INCLUDED
#define CUSS_SDLMONIO_INCLUDED

#include "SDL_events.h"
#include "SDL_surface.h"
#include <stdbool.h>

#include "errors.h"

extern bool CuSdlMonIoSetUp(int scr_width, int scr_height,
  CuError* restrict err);
extern bool CuSdlMonIoTearDown(CuError* restrict err);

extern bool CuSdlMonIoGetInp(char* restrict buf, size_t buf_size,
  bool* restrict eof, CuError* restrict err);
extern bool CuSdlMonIoPutMsg(const char* restrict msg, CuError* restrict err);

extern bool CuSdlMonIoRender(SDL_Surface* restrict screen,
  CuError* restrict err);

extern bool CuSdlMonProcEvt(const SDL_Event* restrict evt,
  CuError* restrict err);

#endif  // CUSS_SDLMONIO_INCLUDED
