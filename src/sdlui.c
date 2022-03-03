// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "sdlui.h"

// NOTE: Sadly, as of at least SDL v2.0.20, SDL_Init() and SDL_Quit() are only
// declared in the jumbo "SDL.h" header-file that includes all the other SDL2
// header-files. Thus we cannot help but bloat this translation-unit.
#include "SDL.h"
#include <stdint.h>

#include "logger.h"
#include "sdlmonio.h"
#include "sdltxt.h"

#define TARGET_FPS 60
#define MAX_FRAME_TIMES 256
#define INVALID_PERF_CTR 0xDEADC0DEDEADC0DELLU

// The logical screen-resolution.
#define SCR_WIDTH 640
#define SCR_HEIGHT 480

typedef struct FrameTimes {
    uint64_t total_ctr;
    uint64_t draw_ctr;
    uint64_t blit_ctr;
    uint64_t flip_ctr;
} FrameTimes;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Surface* screen = NULL;
static SDL_Texture* screen_texture = NULL;

static bool is_monitor_active = true;  // TODO: Set it correctly.

static struct FrameTimes frame_times[MAX_FRAME_TIMES];
static int curr_frame = 0;
static uint64_t min_ctr_per_frame = 0LLU;

static void TryToSetSdlHint(const char* restrict name,
  const char* restrict value) {
    if (SDL_SetHint(name, value) != SDL_TRUE) {
        CuLogWarn("Could not set SDL hint '%s' to '%s'.", name, value);
    }
    CuLogDebug("Successfully set SDL hint '%s' to '%s'.", name, value);
}

static bool CreateWindow(CuError* restrict err) {
    window = SDL_CreateWindow("CUSS (Completely Useless System Simulator)",
      /*x=*/SDL_WINDOWPOS_CENTERED, /*y=*/SDL_WINDOWPOS_CENTERED,
      /*w=*/SCR_WIDTH, /*h=*/SCR_HEIGHT, /*flags=*/0);
    if (window == NULL) {
        return CuErrMsg(err, "Could not create SDL window: %s", SDL_GetError());
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    CuLogDebug("Logical screen-resolution: %dx%d (physical resolution: %dx%d).",
      SCR_WIDTH, SCR_HEIGHT, w, h);

    return true;
}

static bool CreateRenderer(SDL_PixelFormatEnum* restrict pixel_format,
  CuError* restrict err) {
    const uint32_t window_pixel_format = SDL_GetWindowPixelFormat(window);
    if (window_pixel_format == SDL_PIXELFORMAT_UNKNOWN) {
        return CuErrMsg(err, "Could not get the pixel-format of the window: %s",
          SDL_GetError());
    }
    CuLogDebug("Window pixel-format is '%s'.",
      SDL_GetPixelFormatName(window_pixel_format));

    renderer = SDL_CreateRenderer(window, /*index=*/-1,
      /*flags=*/SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        return CuErrMsg(err, "Could not create renderer: %s", SDL_GetError());
    }
    if (SDL_RenderSetLogicalSize(renderer, SCR_WIDTH, SCR_HEIGHT) != 0) {
        return CuErrMsg(err, "Could not set renderer logical size to %dx%d: %s",
          SCR_WIDTH, SCR_HEIGHT, SDL_GetError());
    }

    SDL_RendererInfo rinfo;
    if (SDL_GetRendererInfo(renderer, &rinfo) != 0) {
        return CuErrMsg(err, "Could not get renderer-info: %s", SDL_GetError());
    }
    CuLogDebug("Renderer '%s'.", rinfo.name);
    if (rinfo.num_texture_formats == 0) {
        return CuErrMsg(err, "Could not get renderer texture-formats.");
    }
    bool window_pixel_format_supported = false;
    for (uint32_t i = 0; i < rinfo.num_texture_formats; i++) {
        CuLogDebug("\trenderer_texture_format[%d]='%s'.", i,
          SDL_GetPixelFormatName(rinfo.texture_formats[i]));

        if (rinfo.texture_formats[i] == window_pixel_format) {
            window_pixel_format_supported = true;
        }
    }
    if (window_pixel_format_supported) {
        CuLogDebug("Window pixel-format directly supported by renderer.");
    }

    // IMPORTANT: We create a working surface and texture with the *same*
    // pixel-format as a texture-format supported by the renderer to avoid
    // expensive format-conversion during surface-blitting.
    const uint32_t renderer_pixel_format = window_pixel_format_supported ?
      window_pixel_format : rinfo.texture_formats[0];
    CuLogDebug("Selected renderer pixel-format is '%s'.",
      SDL_GetPixelFormatName(renderer_pixel_format));

    *pixel_format = renderer_pixel_format;
    return true;
}

static bool CreateScreenSurface(SDL_PixelFormatEnum pixel_format,
  CuError* restrict err) {
    int bpp;
    uint32_t rmask, gmask, bmask, amask;
    if (SDL_PixelFormatEnumToMasks(pixel_format, &bpp, &rmask, &gmask,
        &bmask, &amask) != SDL_TRUE) {
        return CuErrMsg(err,
          "Could not unpack the pixel-format of the renderer: %s",
          SDL_GetError());
    }

    screen = SDL_CreateRGBSurface(/*flags=*/0, SCR_WIDTH, SCR_HEIGHT, bpp,
      rmask, gmask, bmask, amask);
    if (screen == NULL) {
        return CuErrMsg(err, "Unable to create screen-surface: %s",
          SDL_GetError());
    }
    CuLogDebug("Screen pixel-format is '%s'.",
      SDL_GetPixelFormatName(screen->format->format));

    screen_texture = SDL_CreateTexture(renderer, pixel_format,
      SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);
    if (screen_texture == NULL) {
        return CuErrMsg(err, "Unable to create screen-texture: %s",
          SDL_GetError());
    }
    SDL_SetTextureBlendMode(screen_texture, SDL_BLENDMODE_NONE);

    uint32_t texture_pixel_format;
    if (SDL_QueryTexture(screen_texture, &texture_pixel_format, /*access=*/NULL,
        /*w=*/NULL, /*h=*/NULL) != 0) {
        return CuErrMsg(err, "Unable to query screen-texture: %s",
          SDL_GetError());
    }
    CuLogDebug("Texture pixel-format is '%s'.",
      SDL_GetPixelFormatName(texture_pixel_format));

    return true;
}

bool CuSdlUiSetUp(CuError* restrict err) {
    TryToSetSdlHint("SDL_HINT_RENDER_DRIVER", "opengl");
    TryToSetSdlHint("SDL_HINT_RENDER_VSYNC", "0");
    TryToSetSdlHint("SDL_HINT_FRAMEBUFFER_ACCELERATION", "1");
    TryToSetSdlHint("SDL_HINT_RENDER_SCALE_QUALITY", "linear");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        return CuErrMsg(err, "Could not initialize SDL2: %s", SDL_GetError());
    }
    atexit(SDL_Quit);

    min_ctr_per_frame = SDL_GetPerformanceFrequency() / TARGET_FPS;

    for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
        CuLogDebug("\tvideo_driver[%d] = %s", i, SDL_GetVideoDriver(i));
    }
    CuLogDebug("Selected video_driver = %s", SDL_GetCurrentVideoDriver());

    RET_ON_ERR(CreateWindow(err));
    SDL_PixelFormatEnum pixel_format = SDL_PIXELFORMAT_UNKNOWN;
    RET_ON_ERR(CreateRenderer(&pixel_format, err));
    RET_ON_ERR(CreateScreenSurface(pixel_format, err));

    for (int i = 0; i < MAX_FRAME_TIMES; i++) {
        frame_times[i].total_ctr = INVALID_PERF_CTR;
        frame_times[i].draw_ctr = INVALID_PERF_CTR;
        frame_times[i].blit_ctr = INVALID_PERF_CTR;
        frame_times[i].flip_ctr = INVALID_PERF_CTR;
    }
    curr_frame = 0;

    RET_ON_ERR(CuSdlTxtSetUp(screen->format, err));
    RET_ON_ERR(CuSdlMonIoSetUp(SCR_WIDTH, SCR_HEIGHT, err));
    return true;
}

static inline uint64_t PerfCtrDeltaToMs(uint64_t delta) {
#define MILLIS_PER_SEC 1000LLU
    return delta * MILLIS_PER_SEC / SDL_GetPerformanceFrequency();
#undef MILLIS_PER_SEC
}

static void PrintRenderTimings(void) {
    uint64_t sum_total_ms = 0;
    uint64_t sum_draw_ms = 0;
    uint64_t sum_blit_ms = 0;
    uint64_t sum_flip_ms = 0;
    int num_frames = 0;

    for (int i = 0; i < MAX_FRAME_TIMES; i++) {
        if (frame_times[i].total_ctr == INVALID_PERF_CTR) {
            continue;
        }
        sum_total_ms += PerfCtrDeltaToMs(frame_times[i].total_ctr);
        sum_draw_ms += PerfCtrDeltaToMs(frame_times[i].draw_ctr);
        sum_blit_ms += PerfCtrDeltaToMs(frame_times[i].blit_ctr);
        sum_flip_ms += PerfCtrDeltaToMs(frame_times[i].flip_ctr);
        num_frames++;
    }
    CuLogInfo("Average render-timings (in ms; last %d frames):", num_frames);
    CuLogInfo("  total=%0.2f (fps=%d), draw=%0.2f, blit=%0.2f, flip=%0.2f",
      (double)sum_total_ms/(double)num_frames,
      (int)(1000.0 / ((double)sum_total_ms/(double)num_frames)),
      (double)sum_draw_ms/(double)num_frames,
      (double)sum_blit_ms/(double)num_frames,
      (double)sum_flip_ms/(double)num_frames);
}

bool CuSdlUiTearDown(void) {
    PrintRenderTimings();

    CuSdlMonIoTearDown();
    CuSdlTxtTearDown();

    SDL_DestroyTexture(screen_texture);
    SDL_FreeSurface(screen);
    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    screen_texture = NULL;
    screen = NULL;
    renderer = NULL;
    window = NULL;

    return true;
}

static bool RenderFrame(CuError* restrict err) {
    const uint64_t t0 = SDL_GetPerformanceCounter();

    if (is_monitor_active) {
        RET_ON_ERR(CuSdlMonIoRender(screen, err));
    }
    const uint64_t t_draw = SDL_GetPerformanceCounter();

    SDL_UpdateTexture(screen_texture, /*rect=*/NULL, screen->pixels,
      screen->pitch);
    const uint64_t t_blit = SDL_GetPerformanceCounter();

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen_texture, /*srcrect=*/NULL,
      /*dstrect=*/NULL);
    SDL_RenderPresent(renderer);
    const uint64_t t_flip = SDL_GetPerformanceCounter();

    const uint64_t total_ctr = t_flip - t0;
    frame_times[curr_frame].total_ctr = total_ctr;
    frame_times[curr_frame].draw_ctr = t_draw - t0;
    frame_times[curr_frame].blit_ctr = t_blit - t_draw;
    frame_times[curr_frame].flip_ctr = t_flip - t_blit;
    curr_frame = (curr_frame + 1) % MAX_FRAME_TIMES;

    if (total_ctr < min_ctr_per_frame) {
        SDL_Delay(PerfCtrDeltaToMs(min_ctr_per_frame - total_ctr));
    }
    return true;
}

bool CuSdlUiRunEventLoop(CuError* restrict err) {
    bool quit = false;
    while (!quit) {
        RET_ON_ERR(RenderFrame(err));

        SDL_Event evt;
        while (SDL_PollEvent(&evt) == 1) {
            switch (evt.type) {
              case SDL_QUIT:
                quit = true;
                break;

              case SDL_KEYUP:
                if (evt.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
                break;

              case SDL_WINDOWEVENT:
                if (evt.window.event == SDL_WINDOWEVENT_CLOSE) {
                    quit = true;
                }
                break;
            }
            if (!quit && is_monitor_active) {
                RET_ON_ERR(CuSdlMonProcEvt(&evt, err));
            }
        }
    }
    return true;
}
