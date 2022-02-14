// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "monitor.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "opdec.h"

static CuMonGetInpFn inp_fn = NULL;
static CuMonPutMsgFn out_fn = NULL;

#define TRY_OUTPUT(msg) \
  do { \
      if (!out_fn((msg), err)) { \
          return false; \
      } \
  } while (false)

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
    TRY_OUTPUT("Commands:\n");
    TRY_OUTPUT("  .: Repeat last command.\n");
    TRY_OUTPUT("  ?, help: Show available commands.\n");
    TRY_OUTPUT("  dis: Disassemble code.\n");
    TRY_OUTPUT("  exit, quit: Exit CUSS.\n");
    TRY_OUTPUT("  reg: Print out register-values.\n");
    TRY_OUTPUT("  step: Execute the next instruction.\n");
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

    TRY_OUTPUT(msg_buf);
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
                TRY_OUTPUT("\n");
            }
            snprintf(msg_buf, MSG_BUF_SIZE, "[r%02d-r%02d]:", i,
              i + REGS_PER_LINE - 1);
            TRY_OUTPUT(msg_buf);
        }
#undef REGS_PER_LINE

        uint32_t rval;
        if (!CuGetIntReg(i, &rval, &nerr)) {
            return CuErrMsg(err, "Error reading register %d: %s", i,
              nerr.err_msg);
        }
        snprintf(msg_buf, MSG_BUF_SIZE, " %08" PRIx32, rval);
        TRY_OUTPUT(msg_buf);
    }
#undef MSG_BUF_SIZE
    TRY_OUTPUT("\n");

    return true;
}

bool CuRunMon(bool* restrict quit, CuError* restrict err) {
    if (inp_fn == NULL || out_fn == NULL) {
        return CuErrMsg(err, "Monitor not initialized.");
    }
    if (quit == NULL) {
        return CuErrMsg(err, "NULL `quit`.");
    }
    TRY_OUTPUT("                *** CUSS Monitor ***\n");
    TRY_OUTPUT("(Enter 'help' to see the available commands.)\n");

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
            TRY_OUTPUT("CUSS > ");
            if (!inp_fn(inp, MAX_USER_INPUT_SIZE, &eof, err)) {
                return false;
            }
        }
#undef MAX_USER_INPUT_SIZE
        if (eof) {
            *quit = true;
            return true;
        }

        if (strncmp(inp, ".", 1) == 0) {
            if (prev_inp[0] == '\0') {
                TRY_OUTPUT("ERROR: No previous command.\n");
                // TODO: Handle the case where the user now enters "." again.
            } else {
                rep_cmd = true;
            }
            continue;
        }
        if (strncmp(inp, "?", 1) == 0 || strncmp(inp, "help", 4) == 0) {
            if (!PrintUsage(err)) {
                return false;
            }
            continue;
        }
        if (strncmp(inp, "dis", 3) == 0) {
            if (!Disassemble(err)) {
                return false;
            }
            continue;
        }
        if (strncmp(inp, "exit", 4) == 0 || strncmp(inp, "quit", 4) == 0) {
            *quit = true;
            if (!CuSetCpuState(CU_CPU_QUITTING, err)) {
                return false;
            }
            return true;
        }
        if (strncmp(inp, "reg", 3) == 0) {
            if (!PrintRegisters(err)) {
                return false;
            }
            continue;
        }
        if (strncmp(inp, "step", 4) == 0) {
            if (!CuExecSingleStep(err)) {
                return false;
            }
            if (!Disassemble(err)) {
                return false;
            }
            continue;
        }
        if (inp[0] != '\0') {
            TRY_OUTPUT("ERROR: Unknown command.\n");
        }
    } while (!*quit);

    return true;
}
