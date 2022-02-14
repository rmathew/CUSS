// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "concur.h"

#include "SDL_error.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include <stddef.h>
#include <stdlib.h>

struct CuThread {
    SDL_Thread* sdl_thr;
};

bool CuThrCreate(CuThreadFn fn, const char* restrict name,
  void* data, CuThread* restrict thr, CuError* restrict err) {
    if (thr == NULL) {
        return CuErrMsg(err, "NULL `thr` argument.");
    }
    *thr = malloc(sizeof (struct CuThread));
    if (*thr == NULL) {
        return CuErrMsg(err, "Unable to allocate thread-data.");
    }
    (*thr)->sdl_thr = SDL_CreateThread(fn, name, data);
    if ((*thr)->sdl_thr == NULL) {
        return CuErrMsg(err, "Unable to create thread: %s", SDL_GetError());
    }
    return true;
}

void CuThrWait(CuThread* restrict thr, int* restrict status) {
    if (thr == NULL || *thr == NULL) {
        return;
    }
    SDL_WaitThread((*thr)->sdl_thr, status);
    free(*thr);
    *thr = NULL;
}
