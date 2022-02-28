// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "sdlmonio.h"

#include "SDL_error.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_timer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "concur.h"
#include "logger.h"
#include "sdltxt.h"

#define MAX_MON_INP 256

static char mon_inp_buf[MAX_MON_INP];
static uint8_t* mon_out_buf = NULL;
static int mon_top_row = 0;
static int mon_curr_row = 0;
static int mon_curr_col = 0;
static int mon_max_rows = 0;
static int mon_max_cols = 0;

static CuMutex mon_inp_mut = NULL;
static CuCondVar mon_inp_cv = NULL;
static bool mon_inp_active = false;
static bool mon_show_cursor = false;

bool CuSdlMonIoSetUp(int scr_width, int scr_height, CuError* restrict err) {
    mon_max_rows = scr_height / CuSdlTxtHeight();
    mon_max_cols = scr_width / CuSdlTxtWidth();
    mon_inp_buf[0] = '\0';
    mon_out_buf = calloc(mon_max_rows * mon_max_cols, sizeof(uint8_t));
    if (mon_out_buf == NULL) {
        return CuErrMsg(err, "Unable to allocate Monitor text-buffer.");
    }
    mon_top_row = 0;
    mon_curr_row = 0;
    mon_curr_col = 0;

    RET_ON_ERR(CuMutCreate(&mon_inp_mut, err));
    RET_ON_ERR(CuCondVarCreate(&mon_inp_cv, err));
    mon_inp_active = false;
    mon_show_cursor = false;
    return true;
}

void CuSdlMonIoTearDown(void) {
    if (mon_inp_active) {
        CuError err;
        if (!CuCondVarSignal(&mon_inp_cv, &err)) {
            CuLogWarn("Could not signal Monitor-thread: %s", err.err_msg);
        }
    }

    mon_inp_buf[0] = '\0';
    free(mon_out_buf);
    mon_top_row = 0;
    mon_curr_row = 0;
    mon_curr_col = 0;

    CuMutDestroy(&mon_inp_mut);
    CuCondVarDestroy(&mon_inp_cv);
    mon_inp_active = false;
    mon_show_cursor = false;
}

static inline uint8_t* CurrMonRowData(void) {
    return mon_out_buf + (mon_curr_row * mon_max_cols);
}

static uint8_t* ScrollMonTxt(void) {
    if (mon_curr_row == (mon_max_rows - 1)) {
        mon_top_row = (mon_top_row + 1) % mon_max_rows;
    }
    mon_curr_row = (mon_curr_row + 1) % mon_max_rows;
    mon_curr_col = 0;
    return CurrMonRowData();
}

static void EmitMonTxt(const char* restrict txt) {
#define MAX_CHARS_TO_EMIT 65536
    int n = 0;
    char c;
    uint8_t* restrict row_data = CurrMonRowData();
    while (n < MAX_CHARS_TO_EMIT && (c = *txt++) != '\0') {
        n++;
        if (c == '\n') {
            row_data = ScrollMonTxt();
            continue;
        }
        row_data[mon_curr_col++] = (uint8_t)c;
        if (mon_curr_col == mon_max_cols) {
            row_data = ScrollMonTxt();
        }
    }
    row_data[mon_curr_col] = 0x00;
#undef MAX_CHARS_TO_EMIT
}

static void UnemitMonTxt(size_t n) {
    uint8_t* restrict row_data = CurrMonRowData();
    for (size_t i = 0; i < n; i++) {
        if (mon_curr_col > 0) {
            row_data[--mon_curr_col] = 0x00;
        } else if (mon_curr_row > 0) {
            mon_curr_row--;
            mon_curr_col = mon_max_cols - 1;
            row_data = CurrMonRowData();
            row_data[mon_curr_col] = 0x00;
        } else {
            return;
        }
    }
}

static uint32_t FlipCursorBlink(uint32_t interval, void* param) {
    (void)param;  // Suppress unused variable warning.
    mon_show_cursor = !mon_show_cursor;
    return mon_inp_active ? interval : 0U;
}

bool CuSdlMonIoGetInp(char* restrict buf, size_t buf_size, bool* restrict eof,
  CuError* restrict err) {
    mon_inp_buf[0] = '\0';
    mon_inp_active = true;
    mon_show_cursor = true;

#define CURSOR_BLINK_MS 500
    const SDL_TimerID timer_id = SDL_AddTimer(CURSOR_BLINK_MS, FlipCursorBlink,
      /*param=*/NULL);
    if (timer_id == 0) {
        return CuErrMsg(err, "Unable to set cursor-blink timer: %s",
          SDL_GetError());
    }
#undef CURSOR_BLINK_MS

    SDL_StartTextInput();
    RET_ON_ERR(CuMutLock(&mon_inp_mut, err));
    RET_ON_ERR(CuCondVarWait(&mon_inp_cv, &mon_inp_mut, err));
    strncpy(buf, mon_inp_buf, buf_size);
    buf[buf_size - 1] = '\0';
    RET_ON_ERR(CuMutUnlock(&mon_inp_mut, err));
    SDL_StopTextInput();

    SDL_RemoveTimer(timer_id);
    mon_show_cursor = false;
    mon_inp_active = false;
    *eof = false;

    return true;
}

bool CuSdlMonIoPutMsg(const char* restrict msg, CuError* restrict err) {
    if (msg == NULL) {
        return CuErrMsg(err, "NULL `msg` argument.");
    }
    EmitMonTxt(msg);
    return true;
}

bool CuSdlMonIoRender(SDL_Surface* restrict screen, CuError* restrict err) {
    SDL_FillRect(screen, /*rect=*/NULL,
      SDL_MapRGBA(screen->format, 0x00, 0x00, 0x8f, 0xff));

    const SDL_Color fg_clr = {.r = 0xff, .g = 0xff, .b = 0x4f, .a = 0xff};
    RET_ON_ERR(CuSdlTxtSetColor(&fg_clr, err));

    for (int i = 0; i < mon_max_rows; i++) {
        const int the_row = (mon_top_row + i) % mon_max_rows;
        uint8_t* the_txt = mon_out_buf + (the_row * mon_max_cols);
        const size_t txt_sz = strlen((const char*)the_txt);
        if (txt_sz == 0U) {
            continue;
        }
        if (mon_inp_active && the_row == mon_curr_row) {
            the_txt[txt_sz - 1] = mon_show_cursor ? 0xdb : 0x20;
        }
        SDL_Point pos = {.x = 0, .y = i * CuSdlTxtHeight()};
        RET_ON_ERR(CuSdlTxtRenderByteSeq(screen, &pos, the_txt, txt_sz, err));
    }
    return true;
}

bool CuSdlMonProcEvt(const SDL_Event* restrict evt, CuError* restrict err) {
    switch (evt->type) {
      case SDL_TEXTINPUT:
        RET_ON_ERR(CuMutLock(&mon_inp_mut, err));
        strncat(mon_inp_buf, evt->text.text, MAX_MON_INP - strlen(mon_inp_buf));
        RET_ON_ERR(CuMutUnlock(&mon_inp_mut, err));
        UnemitMonTxt(1);
        EmitMonTxt(evt->text.text);
        EmitMonTxt(" ");
        break;

      case SDL_KEYUP:
        if (evt->key.keysym.sym == SDLK_RETURN) {
            ScrollMonTxt();
            RET_ON_ERR(CuCondVarSignal(&mon_inp_cv, err));
        }
        break;
    }
    return true;
}
