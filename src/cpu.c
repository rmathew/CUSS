// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "cpu.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "memory.h"
#include "ops.h"

#define NUM_IREGS (1 << 5)
#define DEF_REG_VAL 0xdeadbeefU

#define RESET_VECTOR 0x00000000U

static uint32_t cup_iregs[NUM_IREGS];
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
    cup_iregs[0] = 0;
    for (int i = 1; i < NUM_IREGS; i++) {
        cup_iregs[i] = DEF_REG_VAL;
    }
    CussInitOps();
}

bool CussExecNextInsn(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }

    if (!CussExecuteOp(insn, err)) {
        return false;
    }
    return true;
}

bool CussPrintCpuState(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }
    printf("CPU-State (PC=%08" PRIx32 ", Mem@PC=%08" PRIx32 "):", cup_pc,
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
