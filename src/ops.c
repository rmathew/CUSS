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

// Extract the `shamt` field from an instruction.
#define GET_SHAMT(insn) (uint8_t)(((insn & 0x000007C0U) >> 6) & 0x0000001FU)

// Extract the `imm16` field from an instruction.
#define GET_IMM16(insn) (insn & 0x0000FFFFU)

// Extract the `imm21` field from an instruction.
#define GET_IMM21(insn) (insn & 0x001FFFFFU)

// Extract the `imm26` field from an instruction.
#define GET_IMM26(insn) (insn & 0x03FFFFFFU)

// The type of a function to which the execution of a given instruction is
// delegated using the dispatcher-table `cup_op_executors` below.
typedef bool (*CuOpExecutor)(uint32_t, uint32_t, CuError* restrict);

static CuOpExecutor cup_op_executors[NUM_OP0S];

static void SetCpuIntegerFlags(uint64_t res) {
    const bool negative = (res & 0x0000000080000000U) > 0;
    const bool overflow = (res & 0xFFFFFFFF00000000U) > 0;
    const bool carry = (res & 0x0000000100000000U) > 0;
    CuSetIntegerFlags(negative, overflow, carry, (res == 0));
}

static bool CuExecuteBadOp0xNN(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    return CuErrMsg(err,
      "Bad instruction (op0=%02" PRIx8 " at pc=%08" PRIx32 ").", op0, pc);
}

// op0 = 0x00: an R-type container of many instructions.
static bool CuExecuteOp0x00(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t rt_num = GET_RT(insn);
    uint32_t rs0_val;
    if (!CuGetIntRegister(GET_RS0(insn), &rs0_val, err)) {
        return false;
    }
    uint32_t rs1_val;
    if (!CuGetIntRegister(GET_RS1(insn), &rs1_val, err)) {
        return false;
    }
    uint32_t new_pc = pc + sizeof(uint32_t);
    uint64_t ext_prec_val;

    const uint8_t op1 = GET_OP1(insn);
    switch (op1) {
      case 0x00:
      case 0x01: {
        // SLLR (0x00): Shift left logically using the `rs1` register-operand.
        // SLRF (0x01): The same as SLLR, but sets the integer condition-flags.
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
        // Only consider bits 0-4 - there are only 32 bits in a register.
        rs1_val &= 0x000000001FU;
        ext_prec_val = (uint64_t)rs0_val << rs1_val;
        if (!CuSetIntRegister(rt_num, (uint32_t)(ext_prec_val & 0xFFFFFFFFU),
          err)) {
            return false;
        }
        if (op1 == 0x01) {
            SetCpuIntegerFlags(ext_prec_val);
        }
        break;
      }

      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05: {
        // SRLR (0x02): Shift right logically using the `rs1` register-operand.
        // SRRF (0x03): The same as SRLR, but sets the integer condition-flags.
        // SRAR (0x04): Shift right arithmetic using the `rs1` register-operand.
        // SRAS (0x05): The same as SRAR, but sets the integer condition-flags.

        // Only consider bits 0-4 - there are only 32 bits in a register.
        rs1_val &= 0x000000001FU;
        ext_prec_val = (uint64_t)rs0_val >> rs1_val;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x04 || op1 == 0x05) && (rs0_val & 0x80000000U)) {
            // A string of `rs1_val` 1s in the LSB.
            uint64_t mask = (1 << rs1_val) - 1;
            mask <<= (32 - rs1_val);
            ext_prec_val |= mask;
        }
        if (!CuSetIntRegister(rt_num, (uint32_t)(ext_prec_val & 0xFFFFFFFFU),
          err)) {
            return false;
        }
        if (op1 == 0x03 || op1 == 0x05) {
            SetCpuIntegerFlags(ext_prec_val);
        }
        break;
      }

      case 0x06:
      case 0x07: {
        // SLLI (0x06): Shift left logically using the `shamt` immediate value.
        // SLIF (0x07): The same as SLLI, but sets the integer condition-flags.
        ext_prec_val = (uint64_t)rs0_val << GET_SHAMT(insn);
        if (!CuSetIntRegister(rt_num, (uint32_t)(ext_prec_val & 0xFFFFFFFFU),
          err)) {
            return false;
        }
        if (op1 == 0x07) {
            SetCpuIntegerFlags(ext_prec_val);
        }
        break;
      }

      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b: {
        // SRLI (0x08): Shift right logically using the `shamt` immediate value.
        // SRIF (0x09): The same as SRLI, but sets the integer condition-flags.
        // SRAI (0x0a): Shift right arithmetic with the `shamt` immediate value.
        // SRAJ (0x0b): The same as SRAI, but sets the integer condition-flags.
        const uint8_t shamt = GET_SHAMT(insn);
        ext_prec_val = (uint64_t)rs0_val >> shamt;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x0a || op1 == 0x0b) && (rs0_val & 0x80000000U)) {
            // A string of `shamt` 1s in the LSB.
            uint64_t mask = (1 << shamt) - 1;
            mask <<= (32 - shamt);
            ext_prec_val |= mask;
        }
        if (!CuSetIntRegister(rt_num, (uint32_t)(ext_prec_val & 0xFFFFFFFFU),
          err)) {
            return false;
        }
        if (op1 == 0x09 || op1 == 0x0b) {
            SetCpuIntegerFlags(ext_prec_val);
        }
        break;
      }

      case 0x0c:
      case 0x0d: {
        // ANDR (0x0c): Bit-wise AND of `rs0` and `rs1` operands.
        // ADRF (0x0d): The same as ANDR, but sets the integer condition-flags.
        const uint32_t res = rs0_val & rs1_val;
        if (!CuSetIntRegister(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x0d) {
            SetCpuIntegerFlags(res);
        }
        break;
      }

      case 0x0e:
      case 0x0f: {
        // ORRR (0x0e): Bit-wise OR of `rs0` and `rs1` operands.
        // ORRF (0x0f): The same as ORRR, but sets the integer condition-flags.
        const uint32_t res = rs0_val | rs1_val;
        if (!CuSetIntRegister(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x0f) {
            SetCpuIntegerFlags(res);
        }
        break;
      }

      case 0x10:
      case 0x11: {
        // NOTR (0x10): Bit-wise NOT of `rs0` (`rs1` is ignored).
        // NOTF (0x11): The same as NOTR, but sets the integer condition-flags.
        const uint32_t res = ~rs0_val;
        if (!CuSetIntRegister(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x11) {
            SetCpuIntegerFlags(res);
        }
        break;
      }

      case 0x12:
      case 0x13: {
        // XORR (0x12): Bit-wise XOR of `rs0` and `rs1` operands.
        // XORF (0x13): The same as XORR, but sets the integer condition-flags.
        const uint32_t res = rs0_val ^ rs1_val;
        if (!CuSetIntRegister(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x13) {
            SetCpuIntegerFlags(res);
        }
        break;
      }

      default: {
        return CuErrMsg(err, "Bad instruction (op0=%02" PRIx8 ", op1=%02" PRIx8
          " at pc=%08" PRIx32 ").", 0x00, op1, pc);
      }
    }

    if (!CuSetProgramCounter(new_pc, err)) {
        return false;
    }
    return true;
}

// ANDI: Bit-wise AND with a zero-extended 16-bit immediate value.
static bool CuExecuteOp0x01(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t rt_num = GET_RT(insn);
    uint32_t rs0_val;
    if (!CuGetIntRegister(GET_RS0(insn), &rs0_val, err)) {
        return false;
    }
    const uint32_t imm16 = GET_IMM16(insn);
    if (!CuSetIntRegister(rt_num, rs0_val & imm16, err)) {
        return false;
    }
    if (!CuSetProgramCounter(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

// ORRI: Bit-wise OR with a zero-extended 16-bit immediate value.
static bool CuExecuteOp0x02(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t rt_num = GET_RT(insn);
    uint32_t rs0_val;
    if (!CuGetIntRegister(GET_RS0(insn), &rs0_val, err)) {
        return false;
    }
    const uint32_t imm16 = GET_IMM16(insn);
    if (!CuSetIntRegister(rt_num, rs0_val | imm16, err)) {
        return false;
    }
    if (!CuSetProgramCounter(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

// XORI: Bit-wise XOR with a zero-extended 16-bit immediate value.
static bool CuExecuteOp0x03(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t rt_num = GET_RT(insn);
    uint32_t rs0_val;
    if (!CuGetIntRegister(GET_RS0(insn), &rs0_val, err)) {
        return false;
    }
    const uint32_t imm16 = GET_IMM16(insn);
    if (!CuSetIntRegister(rt_num, rs0_val ^ imm16, err)) {
        return false;
    }
    if (!CuSetProgramCounter(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

void CuInitOps(void) {
    cup_op_executors[0x00] = CuExecuteOp0x00;

    cup_op_executors[0x01] = CuExecuteOp0x01;
    cup_op_executors[0x02] = CuExecuteOp0x02;
    cup_op_executors[0x03] = CuExecuteOp0x03;

    // TODO: Define and implement the rest of the ISA.
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

bool CuExecuteOp(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    if (!cup_op_executors[op0](pc, insn, err)) {
        return false;
    }
    return true;
}
