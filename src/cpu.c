// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "cpu.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "memory.h"
#include "ops.h"

#define NUM_OP0S (1 << 6)

#define NUM_IREGS (1 << 5)
#define DEF_REG_VAL 0xdeadbeefU

#define RESET_VECTOR 0x00000000U

static uint32_t cup_iregs[NUM_IREGS];
static uint32_t cup_pc;

static CuOpExecutor cup_op_executors[NUM_OP0S];

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

    cup_op_executors[0x00] = CuExecuteOp0x00;
    cup_op_executors[0x01] = CuExecuteOp0x01;
    cup_op_executors[0x02] = CuExecuteOp0x02;
    cup_op_executors[0x03] = CuExecuteOp0x03;
    cup_op_executors[0x04] = CuExecuteOp0x04;
    cup_op_executors[0x05] = CuExecuteOp0x05;
    cup_op_executors[0x06] = CuExecuteOp0x06;
    cup_op_executors[0x07] = CuExecuteOp0x07;
    cup_op_executors[0x08] = CuExecuteOp0x08;
    cup_op_executors[0x09] = CuExecuteOp0x09;
    cup_op_executors[0x0a] = CuExecuteOp0x0a;
    cup_op_executors[0x0b] = CuExecuteOp0x0b;
    cup_op_executors[0x0c] = CuExecuteOp0x0c;
    cup_op_executors[0x0d] = CuExecuteOp0x0d;
    cup_op_executors[0x0e] = CuExecuteOp0x0e;
    cup_op_executors[0x0f] = CuExecuteOp0x0f;
    cup_op_executors[0x10] = CuExecuteOp0x10;
    cup_op_executors[0x11] = CuExecuteOp0x11;
    cup_op_executors[0x12] = CuExecuteOp0x12;
    cup_op_executors[0x13] = CuExecuteOp0x13;
    cup_op_executors[0x14] = CuExecuteOp0x14;
    cup_op_executors[0x15] = CuExecuteOp0x15;
    cup_op_executors[0x16] = CuExecuteOp0x16;
    cup_op_executors[0x17] = CuExecuteOp0x17;
    cup_op_executors[0x18] = CuExecuteOp0x18;
    cup_op_executors[0x19] = CuExecuteOp0x19;
    cup_op_executors[0x1a] = CuExecuteOp0x1a;
    cup_op_executors[0x1b] = CuExecuteOp0x1b;
    cup_op_executors[0x1c] = CuExecuteOp0x1c;
    cup_op_executors[0x1d] = CuExecuteOp0x1d;
    cup_op_executors[0x1e] = CuExecuteOp0x1e;
    cup_op_executors[0x1f] = CuExecuteOp0x1f;
    cup_op_executors[0x20] = CuExecuteOp0x20;
    cup_op_executors[0x21] = CuExecuteOp0x21;
    cup_op_executors[0x22] = CuExecuteOp0x22;
    cup_op_executors[0x23] = CuExecuteOp0x23;
    cup_op_executors[0x24] = CuExecuteOp0x24;
    cup_op_executors[0x25] = CuExecuteOp0x25;
    cup_op_executors[0x26] = CuExecuteOp0x26;
    cup_op_executors[0x27] = CuExecuteOp0x27;
    cup_op_executors[0x28] = CuExecuteOp0x28;
    cup_op_executors[0x29] = CuExecuteOp0x29;
    cup_op_executors[0x2a] = CuExecuteOp0x2a;
    cup_op_executors[0x2b] = CuExecuteOp0x2b;
    cup_op_executors[0x2c] = CuExecuteOp0x2c;
    cup_op_executors[0x2d] = CuExecuteOp0x2d;
    cup_op_executors[0x2e] = CuExecuteOp0x2e;
    cup_op_executors[0x2f] = CuExecuteOp0x2f;
    cup_op_executors[0x30] = CuExecuteOp0x30;
    cup_op_executors[0x31] = CuExecuteOp0x31;
    cup_op_executors[0x32] = CuExecuteOp0x32;
    cup_op_executors[0x33] = CuExecuteOp0x33;
    cup_op_executors[0x34] = CuExecuteOp0x34;
    cup_op_executors[0x35] = CuExecuteOp0x35;
    cup_op_executors[0x36] = CuExecuteOp0x36;
    cup_op_executors[0x37] = CuExecuteOp0x37;
    cup_op_executors[0x38] = CuExecuteOp0x38;
    cup_op_executors[0x39] = CuExecuteOp0x39;
    cup_op_executors[0x3a] = CuExecuteOp0x3a;
    cup_op_executors[0x3b] = CuExecuteOp0x3b;
    cup_op_executors[0x3c] = CuExecuteOp0x3c;
    cup_op_executors[0x3d] = CuExecuteOp0x3d;
    cup_op_executors[0x3e] = CuExecuteOp0x3e;
    cup_op_executors[0x3f] = CuExecuteOp0x3f;
}

bool CussExecNextInsn(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }

    uint8_t op0 = (uint8_t)(((insn & 0xFC000000U) >> 26) & 0x0000003FU);
    if (!cup_op_executors[op0](insn, err)) {
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
