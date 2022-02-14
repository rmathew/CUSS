// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "concur.h"
#include "cpu.h"
#include "errors.h"
#include "logger.h"
#include "memory.h"
#include "monitor.h"

#define INVALID_ADDR 0xFFFFFFFFU
#define MAX_ARG_VAL_SIZE 256

typedef struct CuOptions {
    bool info_req;
    char mem_img[MAX_ARG_VAL_SIZE];
    uint32_t break_point;
} CuOptions;

static void PrintUsage(const char* restrict prg) {
    CuLogInfo("The Completely Useless System Simulator (CUSS).");
    CuLogInfo("Usage: %s [options]", prg);
    CuLogInfo("Options:");
    CuLogInfo("  -h, --help: Show this help-message.");
    CuLogInfo("  -b=<addr>, --break-point=<addr>: Break-point at <addr>.");
    CuLogInfo("  -m=<file>, --memory-image=<file>: Load memory-image from "
      "<file>.");
}

static bool ParseCommandLine(int argc, char *argv[], CuOptions* restrict opts,
  CuError* restrict err) {
    opts->info_req = false;
    opts->mem_img[0] = '\0';
    opts->break_point = INVALID_ADDR;
    if (argc < 2) {
        return true;
    }

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (strncmp(arg, "-h", 2) == 0 || strncmp(arg, "--help", 6) == 0) {
            opts->info_req = true;
            PrintUsage(argv[0]);
            return true;
        }
        if (strncmp(arg, "-b=", 3) == 0) {
            opts->break_point = (uint32_t)strtoul(arg + 3, NULL, 0);
            continue;
        }
        if (strncmp(arg, "--break-point=", 14) == 0) {
            opts->break_point = (uint32_t)strtoul(arg + 14, NULL, 0);
            continue;
        }
        if (strncmp(arg, "-m=", 3) == 0) {
            strncpy(opts->mem_img, arg + 3, MAX_ARG_VAL_SIZE - 1);
            continue;
        }
        if (strncmp(arg, "--memory-image=", 15) == 0) {
            strncpy(opts->mem_img, arg + 15, MAX_ARG_VAL_SIZE - 1);
            continue;
        }

        return CuErrMsg(err, "Unexpected argument '%s'.", arg);
    }
    return true;
}

static bool CliGetInp(char* restrict buf, size_t buf_size, bool* restrict eof,
  CuError* restrict err) {
    if (fgets(buf, buf_size, stdin) == NULL) {
        *eof = true;
    } else {
        *eof = false;
    }
    return err != NULL;
}

static bool CliPutMsg(const char* restrict msg, CuError* restrict err) {
    if (fputs(msg, stdout) == EOF) {
        return CuErrMsg(err, "Error writing to `stdout`.");
    }
    return true;
}

static int RunMonitor(void* data) {
    (void)data;  // Suppress unused parameter warning.
    CuError err;
    bool quit = false;
    if (!CuRunMon(&quit, &err)) {
        CuLogError("Could not run the Monitor REPL: %s", err.err_msg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    CuOptions opts;
    CuError err;
    if (!ParseCommandLine(argc, argv, &opts, &err)) {
        CuLogError("Invalid arguments: %s", err.err_msg);
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }
    if (opts.info_req) {
        return EXIT_SUCCESS;
    }

    if (strlen(opts.mem_img) == 0) {
        CuLogError("Missing memory-image file.");
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }
    CuLogInfo("Loading memory-image from file '%s'...", opts.mem_img);
    if (!CuInitMemFromFile(opts.mem_img, &err)) {
        CuLogError("Could not load memory-image file '%s': %s", opts.mem_img,
          err.err_msg);
        return EXIT_FAILURE;
    }

    if (!CuMonSetUp(CliGetInp, CliPutMsg, &err)) {
        CuLogError("Could not initialize the Monitor: %s", err.err_msg);
        return EXIT_FAILURE;
    }

    CuInitCpu();
    if (opts.break_point != INVALID_ADDR) {
        CuLogInfo("Adding a break-point at '%08" PRIx32 "'.", opts.break_point);
        if (!CuAddBreakPoint(opts.break_point, &err)) {
            CuLogError("Unable to add break-point: %s", err.err_msg);
            return EXIT_FAILURE;
        }
    }

    CuLogInfo("Spawing Monitor in a separate thread.");
    CuThread mon_thr;
    if (!CuThrCreate(RunMonitor, "CUSS Monitor", /*data=*/NULL, &mon_thr,
        &err)) {
        CuLogError("Could not spawn a Monitor thread: %s", err.err_msg);
        return EXIT_FAILURE;
    }
    int mon_status;
    CuThrWait(&mon_thr, &mon_status);
    CuLogInfo("Monitor thread finished execution.");
    return mon_status;
}
