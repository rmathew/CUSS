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

struct CuMutex {
    SDL_mutex* sdl_mut;
};

struct CuCondVar {
    SDL_cond* sdl_cond;
};

bool CuThrCreate(CuThreadFn fn, const char* restrict name,
  void* restrict data, CuThread* restrict thr, CuError* restrict err) {
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

bool CuThrWait(CuThread* restrict thr, int* restrict status,
  CuError* restrict err) {
    if (thr == NULL || *thr == NULL || (*thr)->sdl_thr == NULL) {
        return CuErrMsg(err, "Bad `thr` argument.");
    }
    SDL_WaitThread((*thr)->sdl_thr, status);
    free(*thr);
    *thr = NULL;
    return true;
}

bool CuMutCreate(CuMutex* restrict mut, CuError* restrict err) {
    if (mut == NULL) {
        return CuErrMsg(err, "NULL `mut` argument.");
    }
    *mut = malloc(sizeof (struct CuMutex));
    if (*mut == NULL) {
        return CuErrMsg(err, "Unable to allocate mutex.");
    }
    (*mut)->sdl_mut = SDL_CreateMutex();
    if ((*mut)->sdl_mut == NULL) {
        return CuErrMsg(err, "Unable to create mutex: %s", SDL_GetError());
    }
    return true;
}

bool CuMutDestroy(CuMutex* restrict mut, CuError* restrict err) {
    if (mut == NULL || *mut == NULL || (*mut)->sdl_mut == NULL) {
        return CuErrMsg(err, "Bad `mut` argument.");
    }
    SDL_DestroyMutex((*mut)->sdl_mut);
    free(*mut);
    *mut = NULL;
    return true;
}

bool CuMutLock(CuMutex* restrict mut, CuError* restrict err) {
    if (mut == NULL || *mut == NULL || (*mut)->sdl_mut == NULL) {
        return CuErrMsg(err, "Bad `mut` argument.");
    }
    if (SDL_LockMutex((*mut)->sdl_mut) != 0) {
        return CuErrMsg(err, "Unable to lock mutex.");
    }
    return true;
}

bool CuMutUnlock(CuMutex* restrict mut, CuError* restrict err) {
    if (mut == NULL || *mut == NULL || (*mut)->sdl_mut == NULL) {
        return CuErrMsg(err, "Bad `mut` argument.");
    }
    if (SDL_UnlockMutex((*mut)->sdl_mut) != 0) {
        return CuErrMsg(err, "Failed to unlock mutex.");
    }
    return true;
}

bool CuCondVarCreate(CuCondVar* restrict cv, CuError* restrict err) {
    if (cv == NULL) {
        return CuErrMsg(err, "NULL `cv` argument.");
    }
    *cv = malloc(sizeof (struct CuCondVar));
    if (*cv == NULL) {
        return CuErrMsg(err, "Unable to allocate condition-variable.");
    }
    (*cv)->sdl_cond = SDL_CreateCond();
    if ((*cv)->sdl_cond == NULL) {
        return CuErrMsg(err, "Unable to create condition-variable: %s",
          SDL_GetError());
    }
    return true;
}

bool CuCondVarDestroy(CuCondVar* restrict cv, CuError* restrict err) {
    if (cv == NULL || *cv == NULL || (*cv)->sdl_cond == NULL) {
        return CuErrMsg(err, "Bad `cv` argument.");
    }
    SDL_DestroyCond((*cv)->sdl_cond);
    free(*cv);
    *cv = NULL;
    return true;
}

bool CuCondVarWait(CuCondVar* restrict cv, CuMutex* restrict mut,
  CuError* restrict err) {
    if (cv == NULL || *cv == NULL || (*cv)->sdl_cond == NULL) {
        return CuErrMsg(err, "Bad `cv` argument.");
    }
    if (mut == NULL || *mut == NULL || (*mut)->sdl_mut == NULL) {
        return CuErrMsg(err, "Bad `mut` argument.");
    }
    if (SDL_CondWait((*cv)->sdl_cond, (*mut)->sdl_mut) != 0) {
        return CuErrMsg(err, "Failed to wait on condition-variable: %s",
          SDL_GetError());
    }
    return true;
}

bool CuCondVarSignal(CuCondVar* restrict cv, CuError* restrict err) {
    if (cv == NULL || *cv == NULL || (*cv)->sdl_cond == NULL) {
        return CuErrMsg(err, "Bad `cv` argument.");
    }
    if (SDL_CondSignal((*cv)->sdl_cond) != 0) {
        return CuErrMsg(err, "Failed to signal condition-variable: %s",
          SDL_GetError());
    }
    return true;
}
