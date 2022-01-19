// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "cpu.h"

#include <inttypes.h>
#include <stdio.h>

#include "memory.h"
#include "ops.h"

// Number of integer registers and their default values (except for `r0`).
#define NUM_IREGS (1 << 5)
#define DEF_REG_VAL 0xc0def00dU

// What to reset the program-counter to, upon receiving a hard reset signal.
#define RESET_VECTOR 0x00000000U

// The general-purpose integer registers in a CUP core.
static uint32_t cup_iregs[NUM_IREGS];

// An additional register to provide extended precision during multiply/divide.
static uint32_t cup_epr;

// The program-counter of a CUP core.
static uint32_t cup_pc;

// The processor state register of a CUP core.
static uint32_t cup_psr;

static inline bool NextInsn(uint32_t* restrict insn, CuError* restrict err) {
    CuError nerr;
    if (!CuGetWordAt(cup_pc, insn, &nerr)) {
        return CuErrMsg(err, "Error reading instruction: %s", nerr.err_msg);
    }
    return true;
}

void CuInitCpu(void) {
    cup_pc = RESET_VECTOR;
    cup_iregs[0] = 0x00000000U;
    for (int i = 1; i < NUM_IREGS; i++) {
        cup_iregs[i] = DEF_REG_VAL;
    }
    cup_psr = 0x00000000U;
    cup_epr = 0x00000000U;
    CuInitOps();
}

bool CuGetIntReg(uint8_t r_n, uint32_t* restrict r_val, CuError* restrict err) {
    if (r_n >= NUM_IREGS) {
        return CuErrMsg(err, "Bad register (r_n=%02" PRIx8 ").");
    }
    *r_val = cup_iregs[r_n];
    return true;
}

bool CuSetIntReg(uint8_t r_n, uint32_t r_val, CuError* restrict err) {
    if (r_n >= NUM_IREGS) {
        return CuErrMsg(err, "Bad register (r_n=%02" PRIx8 ").");
    }
    if (r_n != 0x00) {
        cup_iregs[r_n] = r_val;
    }
    return true;
}

uint32_t CuGetExtPrecReg() {
    return cup_epr;
}

void CuSetExtPrecReg(uint32_t r_val) {
    cup_epr = r_val;
}

uint32_t CuGetProgCtr(void) {
    return cup_pc;
}

bool CuSetProgCtr(uint32_t pc, CuError* restrict err) {
    if (!CuIsValidPhyMemAddr(pc, err)) {
        return false;
    }
    if (pc & 0x00000003U) {
        return CuErrMsg(err, "Unaligned instruction (PC=%08" PRIx32 ").", pc);
    }
    cup_pc = pc;
    return true;
}

bool CuIsNegFlagSet(void) {
    return (cup_psr & 0x00000008U) > 0;
}

bool CuIsOvfFlagSet(void) {
    return (cup_psr & 0x00000004U) > 0;
}

bool CuIsCarFlagSet(void) {
    return (cup_psr & 0x00000002U) > 0;
}

bool CuIsZerFlagSet(void) {
    return (cup_psr & 0x00000001U) > 0;
}

void CuSetIntFlags(bool neg, bool ovf, bool car, bool zer) {
    if (neg) {
        cup_psr |= 0x00000008U;
    }
    if (ovf) {
        cup_psr |= 0x00000004U;
    }
    if (car) {
        cup_psr |= 0x00000002U;
    }
    if (zer) {
        cup_psr |= 0x00000001U;
    }
}

bool CuExecNextInsn(CuError* restrict err) {
    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        return false;
    }

    if (!CuExecOp(cup_pc, insn, err)) {
        return false;
    }
    return true;
}

bool CuPrintCpuState(CuError* restrict err) {
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
