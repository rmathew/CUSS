// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_SDLTXT_INCLUDED
#define CUSS_SDLTXT_INCLUDED

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"
#include <stdbool.h>
#include <stdint.h>

#include "errors.h"

extern bool CuSdlTxtSetUp(const SDL_PixelFormat* restrict pixel_format,
  CuError* restrict err);
extern void CuSdlTxtTearDown(void);

extern int CuSdlTxtWidth();
extern int CuSdlTxtHeight();

extern bool CuSdlTxtSetColor(const SDL_Color* restrict clr,
  CuError* restrict err);

extern bool CuSdlTxtRenderByte(SDL_Surface* restrict screen,
  const SDL_Point* restrict pos, uint8_t byt, CuError* restrict err);

extern bool CuSdlTxtRender(SDL_Surface* restrict screen,
  const SDL_Point* restrict pos, const char* restrict txt,
  CuError* restrict err);

#endif  // CUSS_SDLTXT_INCLUDED
