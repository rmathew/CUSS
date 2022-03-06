// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CUSS_SDLTXT_INCLUDED
#define CUSS_SDLTXT_INCLUDED

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_surface.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "errors.h"

extern bool CuSdlTxtSetUp(const SDL_PixelFormat* restrict pixel_format,
  CuError* restrict err);
extern bool CuSdlTxtTearDown(CuError* restrict err);

extern int CuSdlTxtWidth();
extern int CuSdlTxtHeight();

extern bool CuSdlTxtSetColor(const SDL_Color* restrict clr,
  CuError* restrict err);

extern bool CuSdlTxtRenderByteSeq(SDL_Surface* restrict screen,
  const SDL_Point* restrict pos, const uint8_t* restrict seq, size_t n,
  CuError* restrict err);

#endif  // CUSS_SDLTXT_INCLUDED
