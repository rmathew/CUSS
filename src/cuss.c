// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "errors.h"
#include "logger.h"
#include "memory.h"
#include "opdec.h"

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
    if (argc != 2) {
        CuLogError("Invalid number of arguments.\n");
        CuLogError("Usage: %s <memory-image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    CuError err;
    CuLogInfo("Loading memory-image from file '%s'...\n", argv[1]);
    if (!CuInitMemFromFile(argv[1], &err)) {
        CuLogError("Could not load '%s': %s\n", argv[1], err.err_msg);
        return EXIT_FAILURE;
    }

    CuInitCpu();
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
