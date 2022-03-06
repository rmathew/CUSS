// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "sdlmonio.h"

#include "SDL_error.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_stdinc.h"
#include "SDL_timer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "concur.h"
#include "logger.h"
#include "sdltxt.h"

// Maximum size of the command-line (including the terminal '\0').
#define MAX_MON_INP 256

static char mon_inp_buf[MAX_MON_INP];
static uint8_t* mon_out_buf = NULL;
static int mon_init_row = 0;
static int mon_curr_row = 0;
static int mon_curr_col = 0;
static int mon_max_rows = 0;
static int mon_max_cols = 0;

static CuMutex mon_inp_mut = NULL;
static CuCondVar mon_inp_cv = NULL;
static bool mon_inp_active = false;
static bool mon_inp_eof = true;
static bool mon_show_cursor = false;

bool CuSdlMonIoSetUp(int scr_width, int scr_height, CuError* restrict err) {
    mon_max_rows = scr_height / CuSdlTxtHeight();
    mon_max_cols = scr_width / CuSdlTxtWidth();
    mon_inp_buf[0] = '\0';
    mon_out_buf = calloc(mon_max_rows * mon_max_cols, sizeof(uint8_t));
    if (mon_out_buf == NULL) {
        return CuErrMsg(err, "Unable to allocate Monitor text-buffer.");
    }
    mon_init_row = 0;
    mon_curr_row = 0;
    mon_curr_col = 0;

    RET_ON_ERR(CuMutCreate(&mon_inp_mut, err));
    RET_ON_ERR(CuCondVarCreate(&mon_inp_cv, err));
    mon_inp_active = false;
    mon_inp_eof = false;
    mon_show_cursor = false;
    return true;
}

bool CuSdlMonIoTearDown(CuError* restrict err) {
    mon_inp_eof = true;
    if (mon_inp_active) {
        RET_ON_ERR(CuCondVarSignal(&mon_inp_cv, err));

#define MON_THR_WAIT_MS 250
#define MAX_MON_THR_WAIT_ATTEMPTS 4

        int n = 0;
        do {
            SDL_Delay(MON_THR_WAIT_MS);
            n++;
        } while (mon_inp_active && n < MAX_MON_THR_WAIT_ATTEMPTS);
#undef MAX_MON_THR_WAIT_ATTEMPTS
#undef MON_THR_WAIT_MS
    }
    if (mon_inp_active) {
        return CuErrMsg(err, "Timed out waiting for Monitor-thread.");
    }
    RET_ON_ERR(CuMutLock(&mon_inp_mut, err));
    RET_ON_ERR(CuCondVarDestroy(&mon_inp_cv, err));
    RET_ON_ERR(CuMutUnlock(&mon_inp_mut, err));
    RET_ON_ERR(CuMutDestroy(&mon_inp_mut, err));

    mon_inp_buf[0] = '\0';
    free(mon_out_buf);
    mon_init_row = 0;
    mon_curr_row = 0;
    mon_curr_col = 0;

    mon_inp_active = false;
    mon_show_cursor = false;
    return true;
}

static inline uint8_t* CurrMonRowData(void) {
    return mon_out_buf + (mon_curr_row * mon_max_cols);
}

static inline int NumMonRows(void) {
    if (mon_init_row > mon_curr_row) {
        return mon_max_rows - mon_init_row + mon_curr_row + 1;
    }
    return mon_curr_row - mon_init_row + 1;
}

static uint8_t* NextMonRowData(void) {
    if (NumMonRows() == mon_max_rows) {
        mon_init_row = (mon_init_row + 1) % mon_max_rows;
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
        if (c == '\n' && mon_curr_col < mon_max_cols) {
            row_data[mon_curr_col] = 0x00;
        }
        if (c == '\n' || mon_curr_col == mon_max_cols) {
            row_data = NextMonRowData();
        }
        if (c != '\n') {
            row_data[mon_curr_col++] = (uint8_t)c;
        }
        n++;
    }
    if (mon_curr_col < mon_max_cols) {
        row_data[mon_curr_col] = 0x00;
    }
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
    if (mon_inp_active) {
        return interval;
    }
    CuLogInfo("Cancelling cursor-blink timer-function.");
    return 0U;
}

bool CuSdlMonIoGetInp(char* restrict buf, size_t buf_size, bool* restrict eof,
  CuError* restrict err) {
    mon_inp_buf[0] = '\0';
    mon_inp_active = true;
    mon_show_cursor = true;

    EmitMonTxt(" ");  // Add a place-holder for the cursor.
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

    if (SDL_RemoveTimer(timer_id) == SDL_FALSE) {
        CuLogWarn("Could not remove cursor-blink timer-function %d.", timer_id);
    }
    mon_show_cursor = false;
    mon_inp_active = false;
    *eof = mon_inp_eof;

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
      SDL_MapRGBA(screen->format, 0x00, 0x5f, 0x87, 0xff));

    const SDL_Color fg_clr = {.r = 0xee, .g = 0xee, .b = 0xee, .a = 0xff};
    RET_ON_ERR(CuSdlTxtSetColor(&fg_clr, err));

    const int num_rows = NumMonRows();
    for (int i = 0; i < num_rows; i++) {
        const int the_row = (mon_init_row + i) % mon_max_rows;
        uint8_t* the_txt = mon_out_buf + (the_row * mon_max_cols);
        if (the_txt[0] == 0x00) {
            continue;
        }
        size_t txt_sz = mon_max_cols;
        const void* first_nil = memchr(the_txt, 0x00, mon_max_cols);
        if (first_nil != NULL) {
            txt_sz = (uint8_t*)first_nil - the_txt;
        }
        if (mon_inp_active && the_row == mon_curr_row && txt_sz > 0) {
            the_txt[txt_sz - 1] = mon_show_cursor ? 0xdb : 0x20;
        }
        SDL_Point pos = {.x = 0, .y = i * CuSdlTxtHeight()};
        RET_ON_ERR(CuSdlTxtRenderByteSeq(screen, &pos, the_txt, txt_sz, err));
    }
    return true;
}

bool CuSdlMonProcEvt(const SDL_Event* restrict evt, CuError* restrict err) {
    if (!mon_inp_active) {
        return true;
    }
    const size_t mon_inp_size = strlen(mon_inp_buf);
    switch (evt->type) {
      case SDL_TEXTINPUT:
        if (mon_inp_size < (MAX_MON_INP - 1)) {
            RET_ON_ERR(CuMutLock(&mon_inp_mut, err));
            strncat(mon_inp_buf, evt->text.text, MAX_MON_INP - mon_inp_size);
            RET_ON_ERR(CuMutUnlock(&mon_inp_mut, err));
            UnemitMonTxt(1);  // Remove the place-holder for the cursor.
            EmitMonTxt(evt->text.text);
            EmitMonTxt(" ");  // Add a place-holder for the cursor.
        }
        break;

      case SDL_KEYUP:
        if (evt->key.keysym.sym == SDLK_RETURN) {
            UnemitMonTxt(1);  // Remove the place-holder for the cursor.
            NextMonRowData();
            RET_ON_ERR(CuCondVarSignal(&mon_inp_cv, err));
        } else if (evt->key.keysym.sym == SDLK_d &&
          (evt->key.keysym.mod & KMOD_CTRL) != 0) {
            UnemitMonTxt(1);  // Remove the place-holder for the cursor.
            NextMonRowData();
            mon_inp_eof = true;
            RET_ON_ERR(CuCondVarSignal(&mon_inp_cv, err));
        }
        break;

      case SDL_KEYDOWN:
        if (evt->key.keysym.sym == SDLK_BACKSPACE && mon_inp_size > 0) {
            UnemitMonTxt(2);  // Remove the place-holder for the cursor as well.
            mon_inp_buf[mon_inp_size - 1] = '\0';
            EmitMonTxt(" ");  // Add a place-holder for the cursor.
        }
        break;
    }
    return true;
}
