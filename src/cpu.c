// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "cpu.h"

#include <inttypes.h>

#include "memory.h"
#include "ops.h"

// The default values in the integer registers (except for `r0`).
#define DEF_REG_VAL 0xC0DEF00DU

// What to reset the program-counter to, upon receiving a hard reset signal.
#define RESET_VECTOR 0x00000000U

// How many break-points to support at a time.
#define MAX_BREAK_POINTS 16
// Sentinel-value for an invalid break-point.
#define INVALID_BREAK_POINT 0xFFFFFFFFU

// The general-purpose integer registers in a CUP core.
static uint32_t cup_iregs[CU_NUM_IREGS];

// An additional register to provide extended precision during multiply/divide.
static uint32_t cup_epr;

// The program-counter of a CUP core.
static uint32_t cup_pc;

// The processor state register of a CUP core.
static uint32_t cup_psr;

static CuCpuState cup_state = CU_CPU_ERROR;

static uint32_t cup_break_points[MAX_BREAK_POINTS];
static int num_break_points = 0;

void CuInitCpu(void) {
    cup_pc = RESET_VECTOR;
    cup_iregs[0] = 0x00000000U;
    for (int i = 1; i < CU_NUM_IREGS; i++) {
        cup_iregs[i] = DEF_REG_VAL;
    }
    cup_psr = 0x00000000U;
    cup_epr = 0x00000000U;

    for (int i = 0; i < MAX_BREAK_POINTS; i++) {
        cup_break_points[i] = INVALID_BREAK_POINT;
    }
    num_break_points = 0;

    CuInitOps();
    cup_state = CU_CPU_PAUSED;
}

CuCpuState CuGetCpuState(void) {
    return cup_state;
}

bool CuGetIntReg(uint8_t r_n, uint32_t* restrict r_val, CuError* restrict err) {
    if (r_n >= CU_NUM_IREGS) {
        return CuErrMsg(err, "Bad register (r_n=%02" PRIx8 ").");
    }
    *r_val = cup_iregs[r_n];
    return true;
}

bool CuSetIntReg(uint8_t r_n, uint32_t r_val, CuError* restrict err) {
    if (r_n >= CU_NUM_IREGS) {
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

static inline bool IsBreakPoint(uint32_t addr) {
    for (int i = 0; i < num_break_points; i++) {
        if (cup_break_points[i] == addr) {
            return true;
        }
    }
    return false;
}

bool CuContinueExec(CuError* restrict err) {
    do {
        if (!CuExecNextInsn(err)) {
            return false;
        }
        if (IsBreakPoint(cup_pc)) {
            cup_state = CU_CPU_BREAK_POINT;
            return true;
        }
    } while (true);
}

static inline bool NextInsn(uint32_t* restrict insn, CuError* restrict err) {
    CuError nerr;
    if (!CuGetWordAt(cup_pc, insn, &nerr)) {
        return CuErrMsg(err, "Error reading next instruction: %s",
          nerr.err_msg);
    }
    return true;
}

bool CuExecNextInsn(CuError* restrict err) {
    cup_state = CU_CPU_RUNNING;

    uint32_t insn;
    if (!NextInsn(&insn, err)) {
        cup_state = CU_CPU_ERROR;
        return false;
    }
    if (!CuExecOp(cup_pc, insn, err)) {
        cup_state = CU_CPU_ERROR;
        return false;
    }

    cup_state = CU_CPU_PAUSED;
    return true;
}

bool CuAddBreakPoint(uint32_t addr, CuError* restrict err) {
    if (!CuIsValidPhyMemAddr(addr, err)) {
        return false;
    }
    for (int i = 0; i < MAX_BREAK_POINTS; i++) {
        if (cup_break_points[i] == INVALID_BREAK_POINT) {
            cup_break_points[i] = addr;
            num_break_points++;
            return true;
        }
    }
    return CuErrMsg(err, "Cannot add any more break-points.");
}

bool CuRemoveBreakPoint(uint32_t addr, CuError* restrict err) {
    int i = 0;
    for (i = 0; i < MAX_BREAK_POINTS; i++) {
        if (cup_break_points[i] == addr) {
            break;
        }
    }
    if (i == MAX_BREAK_POINTS) {
        return CuErrMsg(err, "Could not find break-point '" PRIx32 "'.", addr);
    }
    // Compact the table of break-points for faster searches.
    for (; i < (MAX_BREAK_POINTS - 1); i++) {
        if (cup_break_points[i + 1] == INVALID_BREAK_POINT) {
            break;
        }
        cup_break_points[i] = cup_break_points[i + 1];
    }
    cup_break_points[i] = INVALID_BREAK_POINT;
    num_break_points--;
    return true;
}
