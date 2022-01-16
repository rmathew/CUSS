// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "ops.h"

#include <inttypes.h>

#include "cpu.h"

// The number of MSBs identifying the primary op-code `op0`.
#define NUM_OP0S (1 << 6)

// The number of LSBs identifying the secondary op-code `op1`.
#define NUM_OP1S (1 << 6)

// Extract the `op0`, and `op1` fields from an instruction.
#define GET_OP0(insn) (uint8_t)(((insn & 0xFC000000U) >> 26) & 0x0000003FU)
#define GET_OP1(insn) (uint8_t)((insn) & 0x0000003FU)

// Extract the `rt`, `rs0`, and `rs1` fields from an instruction.
#define GET_RT(insn) (uint8_t)(((insn & 0x03E00000U) >> 21) & 0x0000001FU)
#define GET_RS0(insn) (uint8_t)(((insn & 0x001F0000U) >> 16) & 0x0000001FU)
#define GET_RS1(insn) (uint8_t)(((insn & 0x0000F800U) >> 11) & 0x0000001FU)

// The type of a function to which the execution of a given instruction is
// delegated using the dispatcher-table `cup_op_executors` below.
typedef bool (*CuOpExecutor)(uint32_t, uint32_t, CuError* restrict);

static CuOpExecutor cup_op_executors[NUM_OP0S];

static bool CuExecuteBadOp0xNN(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    return CuErrMsg(err,
      "Bad instruction (op0=%02" PRIx8 " at pc=%08" PRIx32 ").", op0, pc);
}
    
// op0 = 0x00: an R-type instruction for the ALU.
static bool CuExecuteOp0x00(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t new_pc = pc + sizeof(uint32_t);

    const uint8_t rt_num = GET_RT(insn);
    const uint8_t rs0_num = GET_RS0(insn);
    const uint8_t rs1_num = GET_RS1(insn);

    const uint8_t op1 = GET_OP1(insn);
    switch (op1) {
      case 0x00:
      case 0x01:
        // SHLR (0x00): Shift left logically using the `rs1` register-operand.
        // SLRF (0x01): The same as SHLR, but sets the integer condition-flags.
        //
        // NOTE: The advantage of using op0=0x00 and op1=0x00 for the SHLR
        // instruction is that an all-zero bits instruction represents:
        //
        //   SHLR r0, r0, r0
        //
        // which can be conveniently repurposed for a NOP pseudo-instruction by
        // an assembler.
        if (insn == 0x00000000U) {
            break;
        }

        uint32_t rs1_val;
        if (!CussGetIntRegister(rs1_num, &rs1_val, err)) {
            return false;
        }
        // Only consider bits 0-4 - there are only 32 bits in a register.
        rs1_val &= 0x000000001FU;

        uint32_t rs0_val;
        if (!CussGetIntRegister(rs0_num, &rs0_val, err)) {
            return false;
        }

        const uint32_t tmp_val = rs0_val << rs1_val;
        if (!CussSetIntRegister(rt_num, tmp_val, err)) {
            return false;
        }

        // TODO: Implement setting integer condition-flags for SLRF.
        break;

      default:
        return CuErrMsg(err, "Illegal instruction (op0=%02" PRIx8 ", op1=%02"
          PRIx8 ").", 0x00, op1);
    }

    if (!CussSetProgramCounter(new_pc, err)) {
        return false;
    }
    return true;
}

void CussInitOps(void) {
    cup_op_executors[0x00] = CuExecuteOp0x00;

    // TODO: Define and implement the rest of the ISA.
    cup_op_executors[0x01] = CuExecuteBadOp0xNN;
    cup_op_executors[0x02] = CuExecuteBadOp0xNN;
    cup_op_executors[0x03] = CuExecuteBadOp0xNN;
    cup_op_executors[0x04] = CuExecuteBadOp0xNN;
    cup_op_executors[0x05] = CuExecuteBadOp0xNN;
    cup_op_executors[0x06] = CuExecuteBadOp0xNN;
    cup_op_executors[0x07] = CuExecuteBadOp0xNN;
    cup_op_executors[0x08] = CuExecuteBadOp0xNN;
    cup_op_executors[0x09] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0a] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0b] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0c] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0d] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0e] = CuExecuteBadOp0xNN;
    cup_op_executors[0x0f] = CuExecuteBadOp0xNN;
    cup_op_executors[0x10] = CuExecuteBadOp0xNN;
    cup_op_executors[0x11] = CuExecuteBadOp0xNN;
    cup_op_executors[0x12] = CuExecuteBadOp0xNN;
    cup_op_executors[0x13] = CuExecuteBadOp0xNN;
    cup_op_executors[0x14] = CuExecuteBadOp0xNN;
    cup_op_executors[0x15] = CuExecuteBadOp0xNN;
    cup_op_executors[0x16] = CuExecuteBadOp0xNN;
    cup_op_executors[0x17] = CuExecuteBadOp0xNN;
    cup_op_executors[0x18] = CuExecuteBadOp0xNN;
    cup_op_executors[0x19] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1a] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1b] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1c] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1d] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1e] = CuExecuteBadOp0xNN;
    cup_op_executors[0x1f] = CuExecuteBadOp0xNN;
    cup_op_executors[0x20] = CuExecuteBadOp0xNN;
    cup_op_executors[0x21] = CuExecuteBadOp0xNN;
    cup_op_executors[0x22] = CuExecuteBadOp0xNN;
    cup_op_executors[0x23] = CuExecuteBadOp0xNN;
    cup_op_executors[0x24] = CuExecuteBadOp0xNN;
    cup_op_executors[0x25] = CuExecuteBadOp0xNN;
    cup_op_executors[0x26] = CuExecuteBadOp0xNN;
    cup_op_executors[0x27] = CuExecuteBadOp0xNN;
    cup_op_executors[0x28] = CuExecuteBadOp0xNN;
    cup_op_executors[0x29] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2a] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2b] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2c] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2d] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2e] = CuExecuteBadOp0xNN;
    cup_op_executors[0x2f] = CuExecuteBadOp0xNN;
    cup_op_executors[0x30] = CuExecuteBadOp0xNN;
    cup_op_executors[0x31] = CuExecuteBadOp0xNN;
    cup_op_executors[0x32] = CuExecuteBadOp0xNN;
    cup_op_executors[0x33] = CuExecuteBadOp0xNN;
    cup_op_executors[0x34] = CuExecuteBadOp0xNN;
    cup_op_executors[0x35] = CuExecuteBadOp0xNN;
    cup_op_executors[0x36] = CuExecuteBadOp0xNN;
    cup_op_executors[0x37] = CuExecuteBadOp0xNN;
    cup_op_executors[0x38] = CuExecuteBadOp0xNN;
    cup_op_executors[0x39] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3a] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3b] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3c] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3d] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3e] = CuExecuteBadOp0xNN;
    cup_op_executors[0x3f] = CuExecuteBadOp0xNN;
}

bool CussExecuteOp(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    if (!cup_op_executors[op0](pc, insn, err)) {
        return false;
    }
    return true;
}
