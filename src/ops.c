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

// Extract the `rt`, `ra`, and `rb` fields from an instruction.
#define GET_RT(insn) (uint8_t)(((insn & 0x03E00000U) >> 21) & 0x0000001FU)
#define GET_RA(insn) (uint8_t)(((insn & 0x001F0000U) >> 16) & 0x0000001FU)
#define GET_RB(insn) (uint8_t)(((insn & 0x0000F800U) >> 11) & 0x0000001FU)

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

static void SetCpuIntFlags(uint64_t res) {
    const bool neg = (res & 0x0000000080000000U) > 0;
    const bool ovf = (res & 0xFFFFFFFF00000000U) > 0;
    const bool car = (res & 0x0000000100000000U) > 0;
    const bool zer = (res & 0x00000000FFFFFFFFU) == 0;
    CuSetIntFlags(neg, ovf, car, zer);
}

static bool CuExecBadOp0xNN(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    return CuErrMsg(err,
      "Bad instruction (op0=%02" PRIx8 " at pc=%08" PRIx32 ").", op0, pc);
}

// op0 = 0x00: an R-type container of many instructions.
static bool CuExecOp0x00(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t rt_num = GET_RT(insn);
    uint32_t ra_val;
    if (!CuGetIntReg(GET_RA(insn), &ra_val, err)) {
        return false;
    }
    uint32_t rb_val;
    if (!CuGetIntReg(GET_RB(insn), &rb_val, err)) {
        return false;
    }
    uint32_t new_pc = pc + sizeof(uint32_t);
    uint64_t ext_prec_val;

    const uint8_t op1 = GET_OP1(insn);
    switch (op1) {
      case 0x00:
      case 0x01: {
        // SLLR (0x00): Shift `ra` left logically using the `rb` register.
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
        rb_val &= 0x000000001FU;
        ext_prec_val = (uint64_t)ra_val << rb_val;
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x01) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05: {
        // SRLR (0x02): Shift `ra` right logically using the `rb` register.
        // SRRF (0x03): The same as SRLR, but sets the integer condition-flags.
        // SRAR (0x04): Shift `ra` right arithmetic using the `rb` register.
        // SRAS (0x05): The same as SRAR, but sets the integer condition-flags.

        // Only consider bits 0-4 - there are only 32 bits in a register.
        rb_val &= 0x000000001FU;
        ext_prec_val = (uint64_t)ra_val >> rb_val;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x04 || op1 == 0x05) && (ra_val & 0x80000000U)) {
            // A string of `rb_val` 1s in the LSB.
            uint64_t mask = (1 << rb_val) - 1;
            mask <<= (32 - rb_val);
            ext_prec_val |= mask;
        }
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x03 || op1 == 0x05) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x06:
      case 0x07: {
        // SLLI (0x06): Shift `ra` left logically using the `shamt` immediate.
        // SLIF (0x07): The same as SLLI, but sets the integer condition-flags.
        ext_prec_val = (uint64_t)ra_val << GET_SHAMT(insn);
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x07) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b: {
        // SRLI (0x08): Shift `ra` right logically using the `shamt` immediate.
        // SRIF (0x09): The same as SRLI, but sets the integer condition-flags.
        // SRAI (0x0a): Shift `ra` right arithmetic with the `shamt` immediate.
        // SRAJ (0x0b): The same as SRAI, but sets the integer condition-flags.
        const uint8_t shamt = GET_SHAMT(insn);
        ext_prec_val = (uint64_t)ra_val >> shamt;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x0a || op1 == 0x0b) && (ra_val & 0x80000000U)) {
            // A string of `shamt` 1s in the LSB.
            uint64_t mask = (1 << shamt) - 1;
            mask <<= (32 - shamt);
            ext_prec_val |= mask;
        }
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x09 || op1 == 0x0b) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x0c:
      case 0x0d: {
        // ANDR (0x0c): Bit-wise AND of `ra` and `rb` operands.
        // ADRF (0x0d): The same as ANDR, but sets the integer condition-flags.
        const uint32_t res = ra_val & rb_val;
        if (!CuSetIntReg(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x0d) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x0e:
      case 0x0f: {
        // ORRR (0x0e): Bit-wise OR of `ra` and `rb` operands.
        // ORRF (0x0f): The same as ORRR, but sets the integer condition-flags.
        const uint32_t res = ra_val | rb_val;
        if (!CuSetIntReg(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x0f) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x10:
      case 0x11: {
        // NOTR (0x10): Bit-wise NOT of `ra`.
        // NOTF (0x11): The same as NOTR, but sets the integer condition-flags.
        const uint32_t res = ~ra_val;
        if (!CuSetIntReg(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x11) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x12:
      case 0x13: {
        // XORR (0x12): Bit-wise XOR of `ra` and `rb` operands.
        // XORF (0x13): The same as XORR, but sets the integer condition-flags.
        const uint32_t res = ra_val ^ rb_val;
        if (!CuSetIntReg(rt_num, res, err)) {
            return false;
        }
        if (op1 == 0x13) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x14:
      case 0x15: {
        // ADDR (0x14): Addition of `ra` and `rb` operands.
        // ADDF (0x15): The same as ADDR, but sets the integer condition-flags.
        ext_prec_val = ra_val + rb_val;
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x15) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x16:
      case 0x17: {
        // SUBR (0x16): Subtraction of `ra` and `rb` operands.
        // SUBF (0x17): The same as SUBR, but sets the integer condition-flags.
        ext_prec_val = ra_val - rb_val;
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        if (op1 == 0x17) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x18:
      case 0x19: {
        // MULR (0x18): Multiplication of `ra` and `rb` operands.
        // MULF (0x19): The same as MULR, but sets the integer condition-flags.
        ext_prec_val = ra_val * rb_val;
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        CuSetExtPrecReg((ext_prec_val & 0xFFFFFFFF00000000U) >> 32);
        if (op1 == 0x19) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x1a:
      case 0x1b: {
        // DIVR (0x1a): Division of `ep`:`ra` by `rb`.
        // DIVF (0x1b): The same as DIVR, but sets the integer condition-flags.
        uint64_t epra = CuGetExtPrecReg();
        epra <<= 32;
        epra |= ra_val;
        ext_prec_val = epra / rb_val;
        if (!CuSetIntReg(rt_num, ext_prec_val & 0xFFFFFFFFU, err)) {
            return false;
        }
        CuSetExtPrecReg(epra % rb_val);
        if (op1 == 0x1b) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x1c: {
        // RDEP (0x1c): Read `ep` into `rt`.
        if (!CuSetIntReg(rt_num, CuGetExtPrecReg(), err)) {
            return false;
        }
        break;
      }

      case 0x1d: {
        // WREP (0x1d): Write `ep` using `ra`.
        CuSetExtPrecReg(ra_val);
        break;
      }

      default: {
        return CuErrMsg(err, "Bad instruction (op0=%02" PRIx8 ", op1=%02" PRIx8
          " at pc=%08" PRIx32 ").", 0x00, op1, pc);
      }
    }

    if (!CuSetProgCtr(new_pc, err)) {
        return false;
    }
    return true;
}

// ANDI: Bit-wise AND of `ra` with a zero-extended 16-bit immediate value.
static bool CuExecOp0x01(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t ra_val;
    if (!CuGetIntReg(GET_RA(insn), &ra_val, err)) {
        return false;
    }

    uint64_t ext_prec_val = GET_IMM16(insn);
    ext_prec_val &= ra_val;
    if (!CuSetIntReg(GET_RT(insn), ext_prec_val & 0xFFFFFFFFU, err)) {
        return false;
    }
    SetCpuIntFlags(ext_prec_val);

    if (!CuSetProgCtr(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

// ORRI: Bit-wise OR of `ra` with a zero-extended 16-bit immediate value.
static bool CuExecOp0x02(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t ra_val;
    if (!CuGetIntReg(GET_RA(insn), &ra_val, err)) {
        return false;
    }

    uint64_t ext_prec_val = GET_IMM16(insn);
    ext_prec_val |= ra_val;
    if (!CuSetIntReg(GET_RT(insn), ext_prec_val & 0xFFFFFFFFU, err)) {
        return false;
    }
    SetCpuIntFlags(ext_prec_val);

    if (!CuSetProgCtr(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

// XORI: Bit-wise XOR of `ra` with a zero-extended 16-bit immediate value.
static bool CuExecOp0x03(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t ra_val;
    if (!CuGetIntReg(GET_RA(insn), &ra_val, err)) {
        return false;
    }

    uint64_t ext_prec_val = GET_IMM16(insn);
    ext_prec_val ^= ra_val;
    if (!CuSetIntReg(GET_RT(insn), ext_prec_val & 0xFFFFFFFFU, err)) {
        return false;
    }
    SetCpuIntFlags(ext_prec_val);

    if (!CuSetProgCtr(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

// ADDI: Addition of `ra` with a sign-extended 16-bit immediate value.
static bool CuExecOp0x04(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t ra_val;
    if (!CuGetIntReg(GET_RA(insn), &ra_val, err)) {
        return false;
    }

    uint64_t ext_prec_val = GET_IMM16(insn);
    if (ext_prec_val & 0x0000000000008000U) {
        ext_prec_val |= 0xFFFFFFFFFFFF0000U;
    }
    ext_prec_val += ra_val;
    if (!CuSetIntReg(GET_RT(insn), ext_prec_val & 0xFFFFFFFFU, err)) {
        return false;
    }
    SetCpuIntFlags(ext_prec_val);

    if (!CuSetProgCtr(pc + sizeof(uint32_t), err)) {
        return false;
    }
    return true;
}

void CuInitOps(void) {
    cup_op_executors[0x00] = CuExecOp0x00;

    cup_op_executors[0x01] = CuExecOp0x01;
    cup_op_executors[0x02] = CuExecOp0x02;
    cup_op_executors[0x03] = CuExecOp0x03;
    cup_op_executors[0x04] = CuExecOp0x04;

    // TODO: Define and implement the rest of the ISA.
    cup_op_executors[0x05] = CuExecBadOp0xNN;
    cup_op_executors[0x06] = CuExecBadOp0xNN;
    cup_op_executors[0x07] = CuExecBadOp0xNN;
    cup_op_executors[0x08] = CuExecBadOp0xNN;
    cup_op_executors[0x09] = CuExecBadOp0xNN;
    cup_op_executors[0x0a] = CuExecBadOp0xNN;
    cup_op_executors[0x0b] = CuExecBadOp0xNN;
    cup_op_executors[0x0c] = CuExecBadOp0xNN;
    cup_op_executors[0x0d] = CuExecBadOp0xNN;
    cup_op_executors[0x0e] = CuExecBadOp0xNN;
    cup_op_executors[0x0f] = CuExecBadOp0xNN;
    cup_op_executors[0x10] = CuExecBadOp0xNN;
    cup_op_executors[0x11] = CuExecBadOp0xNN;
    cup_op_executors[0x12] = CuExecBadOp0xNN;
    cup_op_executors[0x13] = CuExecBadOp0xNN;
    cup_op_executors[0x14] = CuExecBadOp0xNN;
    cup_op_executors[0x15] = CuExecBadOp0xNN;
    cup_op_executors[0x16] = CuExecBadOp0xNN;
    cup_op_executors[0x17] = CuExecBadOp0xNN;
    cup_op_executors[0x18] = CuExecBadOp0xNN;
    cup_op_executors[0x19] = CuExecBadOp0xNN;
    cup_op_executors[0x1a] = CuExecBadOp0xNN;
    cup_op_executors[0x1b] = CuExecBadOp0xNN;
    cup_op_executors[0x1c] = CuExecBadOp0xNN;
    cup_op_executors[0x1d] = CuExecBadOp0xNN;
    cup_op_executors[0x1e] = CuExecBadOp0xNN;
    cup_op_executors[0x1f] = CuExecBadOp0xNN;
    cup_op_executors[0x20] = CuExecBadOp0xNN;
    cup_op_executors[0x21] = CuExecBadOp0xNN;
    cup_op_executors[0x22] = CuExecBadOp0xNN;
    cup_op_executors[0x23] = CuExecBadOp0xNN;
    cup_op_executors[0x24] = CuExecBadOp0xNN;
    cup_op_executors[0x25] = CuExecBadOp0xNN;
    cup_op_executors[0x26] = CuExecBadOp0xNN;
    cup_op_executors[0x27] = CuExecBadOp0xNN;
    cup_op_executors[0x28] = CuExecBadOp0xNN;
    cup_op_executors[0x29] = CuExecBadOp0xNN;
    cup_op_executors[0x2a] = CuExecBadOp0xNN;
    cup_op_executors[0x2b] = CuExecBadOp0xNN;
    cup_op_executors[0x2c] = CuExecBadOp0xNN;
    cup_op_executors[0x2d] = CuExecBadOp0xNN;
    cup_op_executors[0x2e] = CuExecBadOp0xNN;
    cup_op_executors[0x2f] = CuExecBadOp0xNN;
    cup_op_executors[0x30] = CuExecBadOp0xNN;
    cup_op_executors[0x31] = CuExecBadOp0xNN;
    cup_op_executors[0x32] = CuExecBadOp0xNN;
    cup_op_executors[0x33] = CuExecBadOp0xNN;
    cup_op_executors[0x34] = CuExecBadOp0xNN;
    cup_op_executors[0x35] = CuExecBadOp0xNN;
    cup_op_executors[0x36] = CuExecBadOp0xNN;
    cup_op_executors[0x37] = CuExecBadOp0xNN;
    cup_op_executors[0x38] = CuExecBadOp0xNN;
    cup_op_executors[0x39] = CuExecBadOp0xNN;
    cup_op_executors[0x3a] = CuExecBadOp0xNN;
    cup_op_executors[0x3b] = CuExecBadOp0xNN;
    cup_op_executors[0x3c] = CuExecBadOp0xNN;
    cup_op_executors[0x3d] = CuExecBadOp0xNN;
    cup_op_executors[0x3e] = CuExecBadOp0xNN;
    cup_op_executors[0x3f] = CuExecBadOp0xNN;
}

bool CuExecOp(uint32_t pc, uint32_t insn, CuError* restrict err) {
    const uint8_t op0 = GET_OP0(insn);
    if (!cup_op_executors[op0](pc, insn, err)) {
        return false;
    }
    return true;
}
