// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "monitor.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "opdec.h"

static CuMonGetInpFn inp_fn = NULL;
static CuMonPutMsgFn out_fn = NULL;

bool CuMonSetUp(CuMonGetInpFn get_fn, CuMonPutMsgFn put_fn,
  CuError* restrict err) {
    if (get_fn == NULL) {
        return CuErrMsg(err, "NULL `get_fn`.");
    }
    inp_fn = get_fn;
    if (put_fn == NULL) {
        return CuErrMsg(err, "NULL `put_fn`.");
    }
    out_fn = put_fn;
    return true;
}

static bool PrintUsage(CuError* restrict err) {
    RET_ON_ERR(out_fn("Commands:\n", err));
    RET_ON_ERR(out_fn("  .: Repeat last command.\n", err));
    RET_ON_ERR(out_fn("  ?, help: Show available commands.\n", err));
    RET_ON_ERR(out_fn("  dis: Disassemble code.\n", err));
    RET_ON_ERR(out_fn("  exit, quit: Exit CUSS.\n", err));
    RET_ON_ERR(out_fn("  reg: Print out register-values.\n", err));
    RET_ON_ERR(out_fn("  step: Execute the next instruction.\n", err));
    return true;
}

static bool Disassemble(CuError* restrict err) {
    const uint32_t pc = CuGetProgCtr();

    CuError nerr;
    uint32_t insn;
    if (!CuGetWordAt(pc, &insn, &nerr)) {
        return CuErrMsg(err, "Error reading instruction: %s", nerr.err_msg);
    }

#define INSN_BUF_SIZE 64
    char insn_buf[INSN_BUF_SIZE];
    CuDecodeOp(insn, insn_buf, INSN_BUF_SIZE);
#undef INSN_BUF_SIZE

#define MSG_BUF_SIZE 128
    char msg_buf[MSG_BUF_SIZE];
    snprintf(msg_buf, MSG_BUF_SIZE, "  %08" PRIx32 ": %s\n", pc, insn_buf);
#undef MSG_BUF_SIZE

    RET_ON_ERR(out_fn(msg_buf, err));
    return true;
}

static bool PrintRegisters(CuError* restrict err) {
    CuError nerr;
#define MSG_BUF_SIZE 128
    char msg_buf[MSG_BUF_SIZE];

    for (int i = 0; i < CU_NUM_IREGS; i++) {
#define REGS_PER_LINE 8
        if (i % REGS_PER_LINE == 0) {
            if (i != 0) {
                RET_ON_ERR(out_fn("\n", err));
            }
            snprintf(msg_buf, MSG_BUF_SIZE, "[r%02d-r%02d]:", i,
              i + REGS_PER_LINE - 1);
            RET_ON_ERR(out_fn(msg_buf, err));
        }
#undef REGS_PER_LINE

        uint32_t rval;
        if (!CuGetIntReg(i, &rval, &nerr)) {
            return CuErrMsg(err, "Error reading register %d: %s", i,
              nerr.err_msg);
        }
        snprintf(msg_buf, MSG_BUF_SIZE, " %08" PRIx32, rval);
        RET_ON_ERR(out_fn(msg_buf, err));
    }
#undef MSG_BUF_SIZE
    RET_ON_ERR(out_fn("\n", err));

    return true;
}

static inline bool EqualsStr(const char* restrict s1, const char* restrict s2) {
    const size_t s2_len = strlen(s2);
    return strncmp(s1, s2, s2_len) == 0 && strlen(s1) == s2_len;
}

bool CuRunMon(bool* restrict quit, CuError* restrict err) {
    if (inp_fn == NULL || out_fn == NULL) {
        return CuErrMsg(err, "Monitor not initialized.");
    }
    if (quit == NULL) {
        return CuErrMsg(err, "NULL `quit`.");
    }
    RET_ON_ERR(out_fn("                *** CUSS Monitor ***\n", err));
    RET_ON_ERR(out_fn("(Enter 'help' to see the available commands.)\n", err));

    *quit = false;
#define MAX_USER_INPUT_SIZE 256
    char inp[MAX_USER_INPUT_SIZE];
    inp[0] = '\0';
    char prev_inp[MAX_USER_INPUT_SIZE];
    prev_inp[0] = '\0';
    bool rep_cmd = false;
    do {
        bool eof = false;
        if (rep_cmd) {
            strncpy(inp, prev_inp, MAX_USER_INPUT_SIZE);
            rep_cmd = false;
        } else {
            strncpy(prev_inp, inp, MAX_USER_INPUT_SIZE);
            RET_ON_ERR(out_fn("CUSS > ", err));
            RET_ON_ERR(inp_fn(inp, MAX_USER_INPUT_SIZE, &eof, err));
        }
#undef MAX_USER_INPUT_SIZE
        if (eof) {
            RET_ON_ERR(out_fn("\n-*- EOF -*-\n", err));
            *quit = true;
            RET_ON_ERR(CuSetCpuState(CU_CPU_QUITTING, err));
            return true;
        }

        if (EqualsStr(inp, ".")) {
            if (prev_inp[0] == '\0') {
                RET_ON_ERR(out_fn("ERROR: No previous command.\n", err));
                // TODO: Handle the case where the user now enters "." again.
            } else {
                rep_cmd = true;
            }
            continue;
        }
        if (EqualsStr(inp, "?") || EqualsStr(inp, "help")) {
            RET_ON_ERR(PrintUsage(err));
            continue;
        }
        if (EqualsStr(inp, "dis")) {
            RET_ON_ERR(Disassemble(err));
            continue;
        }
        if (EqualsStr(inp, "exit") || EqualsStr(inp, "quit")) {
            *quit = true;
            RET_ON_ERR(CuSetCpuState(CU_CPU_QUITTING, err));
            return true;
        }
        if (EqualsStr(inp, "reg")) {
            RET_ON_ERR(PrintRegisters(err));
            continue;
        }
        if (EqualsStr(inp, "step")) {
            RET_ON_ERR(CuExecSingleStep(err));
            RET_ON_ERR(Disassemble(err));
            continue;
        }
        if (inp[0] != '\0') {
            RET_ON_ERR(out_fn("ERROR: Unknown command.\n", err));
        }
    } while (!*quit);

    return true;
}
