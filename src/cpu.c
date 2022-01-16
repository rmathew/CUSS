// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "cpu.h"

#include <inttypes.h>
#include <stdio.h>

#include "memory.h"
#include "ops.h"

// Number of integer registers and their default values (except for `r0`).
#define NUM_IREGS (1 << 5)
#define DEF_REG_VAL 0xdeadbeefU

// What to reset the program-counter to, upon receiving a hard reset signal.
#define RESET_VECTOR 0x00000000U

// The general-purpose integer registers in a CUP core.
static uint32_t cup_iregs[NUM_IREGS];

// The program-counter of a CUP core.
static uint32_t cup_pc;

static inline bool NextInsn(uint32_t* restrict insn, CuError* restrict err) {
    CuError nerr;
    if (!CussWordAt(cup_pc, insn, &nerr)) {
        return CuErrMsg(err, "Error reading instruction: %s", nerr.err_msg);
    }
    return true;
}

void CussInitCpu(void) {
    cup_pc = RESET_VECTOR;
    cup_iregs[0] = 0x00000000U;
    for (int i = 1; i < NUM_IREGS; i++) {
        cup_iregs[i] = DEF_REG_VAL;
    }
    CussInitOps();
}

bool CussGetIntRegister(uint8_t r_n, uint32_t* restrict r_val,
  CuError* restrict err) {
    if (r_n >= NUM_IREGS) {
        return CuErrMsg(err, "Bad register (r_n=%02" PRIx8 ").");
    }
    *r_val = cup_iregs[r_n];
    return true;
}

bool CussSetIntRegister(uint8_t r_n, uint32_t r_val,
  CuError* restrict err) {
    if (r_n >= NUM_IREGS) {
        return CuErrMsg(err, "Bad register (r_n=%02" PRIx8 ").");
    }
    if (r_n != 0x00) {
        cup_iregs[r_n] = r_val;
    }
    return true;
}

uint32_t CussGetProgramCounter(void) {
    return cup_pc;
}

bool CussSetProgramCounter(uint32_t pc, CuError* restrict err) {
    if (!CussIsValidPhyMemAddr(pc, err)) {
        return false;
    }
    if (pc & 0x00000003U) {
        return CuErrMsg(err, "Unaligned instruction (PC=%08" PRIx32 ").", pc);
    }
    cup_pc = pc;
    return true;
}

bool CussExecNextInsn(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }

    if (!CussExecuteOp(cup_pc, insn, err)) {
        return false;
    }
    return true;
}

bool CussPrintCpuState(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }
    printf("CPU-State (PC=%08" PRIx32 ", Insn@PC=%08" PRIx32 "):", cup_pc,
      insn);
    for (int i = 0; i < NUM_IREGS; i++) {
#define REGS_PER_LINE 8
        if (i % REGS_PER_LINE == 0) {
            printf("\n[r%02d-r%02d]:", i, i + REGS_PER_LINE - 1);
        }
        printf(" %08" PRIx32 "", cup_iregs[i]);
#undef REGS_PER_LINE
    }
    putchar('\n');
    return true;
}
