// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
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

#define RET_FAIL_ON_ERR(e) \
  do { \
      if (!(e)) { \
          return EXIT_FAILURE; \
      } \
  } while (false)

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

static bool ParseCommandLine(int argc, char *argv[], CuOptions* restrict opts) {
    opts->info_req = false;
    opts->mem_img[0] = '\0';
    opts->break_point = INVALID_ADDR;
    if (argc < 2) {
        return true;
    }

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (strncmp(arg, "-h", 2) == 0 && strlen(arg) == 2) {
            opts->info_req = true;
            PrintUsage(argv[0]);
            return true;
        }
        if (strncmp(arg, "--help", 6) == 0 && strlen(arg) == 6) {
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

        CuLogError("Unexpected argument '%s'.", arg);
        PrintUsage(argv[0]);
        return false;
    }
    return true;
}

static bool MemorySetUp(const CuOptions* restrict opts,
  const char* restrict prg) {
    CuError err;
    if (strlen(opts->mem_img) == 0) {
        CuLogError("Missing memory-image file.");
        PrintUsage(prg);
        return false;
    }
    CuLogInfo("Loading memory-image from file '%s'...", opts->mem_img);
    if (!CuInitMemFromFile(opts->mem_img, &err)) {
        CuLogError("Could not load memory-image file '%s': %s", opts->mem_img,
          err.err_msg);
        return false;
    }
    return true;
}

static bool CpuSetUp(const CuOptions* restrict opts) {
    CuError err;
    if (!CuInitCpu(&err)) {
        CuLogError("Could not initialize the CPU: %s", err.err_msg);
        return false;
    }
    if (opts->break_point != INVALID_ADDR) {
        CuLogInfo("Adding a break-point at '%08" PRIx32 "'.",
          opts->break_point);
        if (!CuAddBreakPoint(opts->break_point, &err)) {
            CuLogError("Unable to add break-point: %s", err.err_msg);
            return false;
        }
    }
    return true;
}

static bool CliGetInp(char* restrict buf, size_t buf_size, bool* restrict eof,
  CuError* restrict err) {
    (void)err;  // Suppress unused parameter warning.
    if (fgets(buf, buf_size, stdin) == NULL) {
        *eof = true;
    } else {
        *eof = false;
    }
    buf[buf_size - 1] = '\0';
    const size_t inp_len = strlen(buf);
    if (buf[inp_len - 1] == '\n') {
        buf[inp_len - 1] = '\0';
    }
    return true;
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

static bool MonitorSetUp(CuThread* restrict mon_thr) {
    CuError err;
    if (!CuMonSetUp(CliGetInp, CliPutMsg, &err)) {
        CuLogError("Could not initialize the Monitor: %s", err.err_msg);
        return false;
    }
    CuLogInfo("Spawning the Monitor in a separate thread.");
    if (!CuThrCreate(RunMonitor, "CUSS Monitor", /*data=*/NULL, mon_thr,
        &err)) {
        CuLogError("Could not spawn a Monitor thread: %s", err.err_msg);
        return false;
    }
    return true;
}

static bool MonitorTearDown(CuThread* restrict mon_thr) {
    int mon_status;
    CuThrWait(mon_thr, &mon_status);
    const bool mon_succ = (mon_status == EXIT_SUCCESS);
    CuLogInfo("Monitor thread finished execution (%s).", mon_succ ? "SUCCESS" :
      "FAILURE");
    return mon_succ;
}

static int RunExecutor(void* data) {
    (void)data;  // Suppress unused parameter warning.
    CuError err;
    if (!CuRunExecution(&err)) {
        CuLogError("Could not run the Simulator: %s", err.err_msg);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static bool ExecutorSetUp(CuThread* restrict exe_thr) {
    CuError err;
    CuLogInfo("Spawning the Executor in a separate thread.");
    if (!CuThrCreate(RunExecutor, "CUSS Executor", /*data=*/NULL, exe_thr,
        &err)) {
        CuLogError("Could not spawn an Executor thread: %s", err.err_msg);
        return false;
    }
    return true;
}

static bool ExecutorTearDown(CuThread* restrict exe_thr) {
    int exe_status;
    CuThrWait(exe_thr, &exe_status);
    const bool exe_succ = (exe_status == EXIT_SUCCESS);
    CuLogInfo("Executor thread finished execution (%s).", exe_succ ?
      "SUCCESS" : "FAILURE");
    return exe_succ;
}

int main(int argc, char *argv[]) {
    CuOptions opts;
    RET_FAIL_ON_ERR(ParseCommandLine(argc, argv, &opts));
    if (opts.info_req) {
        return EXIT_SUCCESS;
    }

    RET_FAIL_ON_ERR(MemorySetUp(&opts, argv[0]));
    RET_FAIL_ON_ERR(CpuSetUp(&opts));

    CuThread mon_thr;
    RET_FAIL_ON_ERR(MonitorSetUp(&mon_thr));
    CuThread exe_thr;
    RET_FAIL_ON_ERR(ExecutorSetUp(&exe_thr));

    RET_FAIL_ON_ERR(ExecutorTearDown(&exe_thr));
    RET_FAIL_ON_ERR(MonitorTearDown(&mon_thr));
    return EXIT_SUCCESS;
}
