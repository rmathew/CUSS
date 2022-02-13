// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "errors.h"
#include "logger.h"
#include "memory.h"
#include "opdec.h"

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

static bool LogCpuState(CuError* restrict err) {
    const uint32_t pc = CuGetProgCtr();

    CuError nerr;
    uint32_t insn;
    if (!CuGetWordAt(pc, &insn, &nerr)) {
        return CuErrMsg(err, "Error reading instruction: %s", nerr.err_msg);
    }

#define MSG_BUF_SIZE 1024
    char msg_buf[MSG_BUF_SIZE];

    CuDecodeOp(insn, msg_buf, MSG_BUF_SIZE);
    CuLogInfo("CPU-State: PC=%08" PRIx32 ", Insn@PC='%s'", pc, msg_buf);

    char* buf_ptr = msg_buf;
    int nc = 0;
    for (int i = 0; i < CU_NUM_IREGS; i++) {
        int np = 0;
#define REGS_PER_LINE 8
        if (i % REGS_PER_LINE == 0) {
            np = snprintf(buf_ptr, MSG_BUF_SIZE - nc, "\n[r%02d-r%02d]:", i,
              i + REGS_PER_LINE - 1);
            nc += np + 1;
        }
#undef REGS_PER_LINE
        if (nc >= MSG_BUF_SIZE) {
            break;
        }
        buf_ptr += np;

        uint32_t rval;
        if (!CuGetIntReg(i, &rval, &nerr)) {
            return CuErrMsg(err, "Error reading register %d: %s", i,
              nerr.err_msg);
        }
        np = snprintf(buf_ptr, MSG_BUF_SIZE - nc - 1, " %08" PRIx32, rval);
        nc += np + 1;
        if (nc >= MSG_BUF_SIZE) {
            break;
        }
        buf_ptr += np;
    }
    if (nc < (MSG_BUF_SIZE - 1)) {
        msg_buf[nc++] = '\n';
        msg_buf[nc++] = '\0';
    }
    CuLogInfo("Register-Values: %s", msg_buf);
    return true;
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
    CuLogInfo("Loading memory-image from file '%s'...\n", opts.mem_img);
    if (!CuInitMemFromFile(opts.mem_img, &err)) {
        CuLogError("Could not load memory-image file '%s': %s\n", opts.mem_img,
          err.err_msg);
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

    do {
        if (!LogCpuState(&err)) {
            CuLogError("Could not log CPU-state: %s\n", err.err_msg);
            return EXIT_FAILURE;
        }
        CuLogInfo("Press ENTER to continue...");
        fflush(stdout);
        getchar();

        if (!CuExecNextInsn(&err)) {
            CuLogError("Execution-fault: %s\n", err.err_msg);
            return EXIT_FAILURE;
        }
    } while (true);

    return EXIT_SUCCESS;
}
