// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "ops.h"

#include <inttypes.h>

#include "cpu.h"
#include "memory.h"

// The register used to establish linkage across procedure-calls.
#define LINK_REG_NUM 31

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

// Extract the `imm5` field from an instruction.
#define GET_IMM5(insn) (uint8_t)(((insn & 0x000007C0U) >> 6) & 0x0000001FU)

// Extract the `imm16` field from an instruction.
#define GET_IMM16(insn) (insn & 0x0000FFFFU)

// Extract the `imm21` field from an instruction.
#define GET_IMM21(insn) (insn & 0x001FFFFFU)

// Extract the `imm26` field from an instruction.
#define GET_IMM26(insn) (insn & 0x03FFFFFFU)

// The usual next value for the program-counter register.
#define NEXT_PC(pc) ((pc) + sizeof(uint32_t))

#define TRY_GET_IREG(reg, val, err) \
  do { \
    if (!CuGetIntReg((reg), (val), (err))) { \
        return false; \
    } \
  } while(false)

#define TRY_SET_IREG(reg, val, err) \
  do { \
    if (!CuSetIntReg((reg), (val), (err))) { \
        return false; \
    } \
  } while(false)

#define TRY_SET_PC(new_pc, err) \
  do { \
    if (!CuSetProgCtr((new_pc), (err))) { \
        return false; \
    } \
  } while (false)


// The type of a function to which the execution of a given instruction is
// delegated using the dispatcher-table `cup_op_executors` below.
typedef bool (*CuOpExecutor)(uint32_t, uint32_t, CuError* restrict);

static CuOpExecutor cup_op_executors[NUM_OP0S];

static inline uint32_t GetSignExtImm16(uint32_t insn) {
    uint32_t imm16 = GET_IMM16(insn);
    if (imm16 & 0x00008000U) {
        imm16 |= 0xFFFF0000U;
    }
    return imm16;
}

static inline uint32_t GetSignExtImm21(uint32_t insn) {
    uint32_t imm21 = GET_IMM21(insn);
    if (imm21 & 0x00100000U) {
        imm21 |= 0xFFE00000U;
    }
    return imm21;
}

static inline uint32_t GetSignExtImm26(uint32_t insn) {
    uint32_t imm26 = GET_IMM26(insn);
    if (imm26 & 0x02000000U) {
        imm26 |= 0xFC000000U;
    }
    return imm26;
}

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
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);

    uint32_t rb_val;
    TRY_GET_IREG(GET_RB(insn), &rb_val, err);

    uint32_t new_pc = NEXT_PC(pc);

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
        const uint32_t res = ra_val << rb_val;
        TRY_SET_IREG(rt_num, res, err);
        if (op1 == 0x01) {
            SetCpuIntFlags(res);
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
        uint32_t res = ra_val >> rb_val;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x04 || op1 == 0x05) && (ra_val & 0x80000000U)) {
            // A string of `rb_val` 1s in the LSB.
            uint64_t mask = (1 << rb_val) - 1;
            mask <<= (32 - rb_val);
            res |= mask;
        }
        TRY_SET_IREG(rt_num, res, err);
        if (op1 == 0x03 || op1 == 0x05) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x06:
      case 0x07: {
        // SLLI (0x06): Shift `ra` left logically using the `imm5` immediate.
        // SLIF (0x07): The same as SLLI, but sets the integer condition-flags.
        const uint32_t res = ra_val << GET_IMM5(insn);
        TRY_SET_IREG(rt_num, res, err);
        if (op1 == 0x07) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b: {
        // SRLI (0x08): Shift `ra` right logically using the `imm5` immediate.
        // SRIF (0x09): The same as SRLI, but sets the integer condition-flags.
        // SRAI (0x0a): Shift `ra` right arithmetic with the `imm5` immediate.
        // SRAJ (0x0b): The same as SRAI, but sets the integer condition-flags.
        const uint8_t imm5 = GET_IMM5(insn);
        uint32_t res = ra_val >> imm5;
        // NOTE: Right shifts for unsigned types in C99 are logical shifts.
        // We therefore need to manually propagate the sign-bit.
        if ((op1 == 0x0a || op1 == 0x0b) && (ra_val & 0x80000000U)) {
            // A string of `imm5` 1s in the LSB.
            uint64_t mask = (1 << imm5) - 1;
            mask <<= (32 - imm5);
            res |= mask;
        }
        TRY_SET_IREG(rt_num, res, err);
        if (op1 == 0x09 || op1 == 0x0b) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x0c:
      case 0x0d: {
        // ANDR (0x0c): Bit-wise AND of `ra` and `rb` operands.
        // ADRF (0x0d): The same as ANDR, but sets the integer condition-flags.
        const uint32_t res = ra_val & rb_val;
        TRY_SET_IREG(rt_num, res, err);
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
        TRY_SET_IREG(rt_num, res, err);
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
        TRY_SET_IREG(rt_num, res, err);
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
        TRY_SET_IREG(rt_num, res, err);
        if (op1 == 0x13) {
            SetCpuIntFlags(res);
        }
        break;
      }

      case 0x14:
      case 0x15: {
        // ADDR (0x14): Addition of `ra` and `rb` operands.
        // ADDF (0x15): The same as ADDR, but sets the integer condition-flags.
        const int64_t ext_prec_val = (int64_t)ra_val + (int64_t)rb_val;
        TRY_SET_IREG(rt_num, ext_prec_val & 0xFFFFFFFFU, err);
        if (op1 == 0x15) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x16:
      case 0x17: {
        // SUBR (0x16): Subtraction of `ra` and `rb` operands.
        // SUBF (0x17): The same as SUBR, but sets the integer condition-flags.
        const int64_t ext_prec_val = (int64_t)ra_val - (int64_t)rb_val;
        TRY_SET_IREG(rt_num, ext_prec_val & 0xFFFFFFFFU, err);
        if (op1 == 0x17) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x18:
      case 0x19: {
        // MULR (0x18): Multiplication of `ra` and `rb` operands.
        // MULF (0x19): The same as MULR, but sets the integer condition-flags.
        const int64_t ext_prec_val = (int64_t)ra_val * (int64_t)rb_val;
        TRY_SET_IREG(rt_num, ext_prec_val & 0xFFFFFFFFU, err);
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
        int64_t epra = CuGetExtPrecReg();
        epra <<= 32;
        epra |= ra_val;
        const int64_t ext_prec_val = epra / (int64_t)rb_val;
        TRY_SET_IREG(rt_num, ext_prec_val & 0xFFFFFFFFU, err);
        CuSetExtPrecReg(epra % (int64_t)rb_val);
        if (op1 == 0x1b) {
            SetCpuIntFlags(ext_prec_val);
        }
        break;
      }

      case 0x1c: {
        // RDEP (0x1c): Read `ep` into `rt`.
        TRY_SET_IREG(rt_num, CuGetExtPrecReg(), err);
        break;
      }

      case 0x1d: {
        // WREP (0x1d): Write `ep` using `ra`.
        CuSetExtPrecReg(ra_val);
        break;
      }

      case 0x1e:
      case 0x1f: {
        // JMPR (0x1e): Jump to the address `ra` + (`rb` << `imm5`).
        // JALR (0x1f): Like JMPR above, but saves the return-address in `r31`.
        uint32_t res = rb_val;
        res <<= GET_IMM5(insn);
        res += ra_val;  // Wrap-around semantics with over-/under-flow.
        if (op1 == 0x1f) {
            TRY_SET_IREG(LINK_REG_NUM, NEXT_PC(pc), err);
        }
        new_pc = res;
        break;
      }

      default: {
        return CuErrMsg(err, "Bad instruction (op0=%02" PRIx8 ", op1=%02" PRIx8
          " at pc=%08" PRIx32 ").", 0x00, op1, pc);
      }
    }

    TRY_SET_PC(new_pc, err);
    return true;
}

// ANDI (0x01): Bit-wise AND of `ra` with a zero-extended 16-bit immediate.
// ORRI (0x02): Bit-wise OR of `ra` with a zero-extended 16-bit immediate.
// XORI (0x03): Bit-wise XOR of `ra` with a zero-extended 16-bit immediate.
static bool CuExecBoolImmOps(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    uint32_t ra_val;
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);

    uint32_t res = GET_IMM16(insn);
    switch (GET_OP0(insn)) {
      case 0x01:
        res &= ra_val;
        break;

      case 0x02:
        res |= ra_val;
        break;

      case 0x03:
        res ^= ra_val;
        break;
    }
    TRY_SET_IREG(GET_RT(insn), res, err);
    SetCpuIntFlags(res);

    TRY_SET_PC(NEXT_PC(pc), err);
    return true;
}

// ADDI (0x04): Addition of `ra` with a sign-extended 16-bit immediate value.
static bool CuExecAddImmOp(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t ra_val;
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);

    int64_t ext_prec_val = GetSignExtImm16(insn);
    ext_prec_val += (int64_t)ra_val;
    TRY_SET_IREG(GET_RT(insn), ext_prec_val & 0xFFFFFFFFU, err);
    SetCpuIntFlags(ext_prec_val);

    TRY_SET_PC(NEXT_PC(pc), err);
    return true;
}

// JMPI (0x05): Jump to a PC-relative address using a sign-extended 26-bit
// immediate value taken as a word-address (giving a 28-bit reach).
// JALI (0x06): Like JMPI above, but saves the return-address in `r31`.
static bool CuExecJmpOps(uint32_t pc, uint32_t insn, CuError* restrict err) {
    uint32_t addr = GetSignExtImm26(insn);
    addr <<= 2;
    addr += pc;  // Wrap-around semantics with over-/under-flow.
    if (GET_OP0(insn) == 0x06) {
        TRY_SET_IREG(LINK_REG_NUM, NEXT_PC(pc), err);
    }
    TRY_SET_PC(addr, err);
    return true;
}

// BRNR (0x07): Jump to the address at `rt` + sign-extended `imm21` (taken as a
// word-address) when the `negative` integer flag is set.
// BROR (0x08): Like BRNR, but for the `overflow` flag.
// BRCR (0x09): Like BRNR, but for the `carry` flag.
// BRZR (0x0a): Like BRNR, but for the `zero` flag.
static bool CuExecFlagBranchOps(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    uint32_t rt_val;
    TRY_GET_IREG(GET_RT(insn), &rt_val, err);

    uint32_t addr = GetSignExtImm21(insn);
    addr <<= 2;
    addr += rt_val;  // Wrap-around semantics with over-/under-flow.

    bool flag_set = false;
    switch (GET_OP0(insn)) {
      case 0x07:
        flag_set = CuIsNegFlagSet();
        break;

      case 0x08:
        flag_set = CuIsOvfFlagSet();
        break;

      case 0x09:
        flag_set = CuIsCarFlagSet();
        break;

      case 0x0a:
        flag_set = CuIsZerFlagSet();
        break;
    }
    const uint32_t new_pc = flag_set ?  addr : (NEXT_PC(pc));
    TRY_SET_PC(new_pc, err);
    return true;
}

// BRNE (0x0b): Jump to the PC-relative address at sign-extended `imm16` (taken
// as a word-address) when `rt` != `ra`.
// BRGT (0x0c): Like BRNE, but when `rt` > `ra`.
static bool CuExecCmpBranchOps(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    uint32_t rt_val;
    TRY_GET_IREG(GET_RT(insn), &rt_val, err);

    uint32_t ra_val;
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);

    uint32_t addr = GetSignExtImm16(insn);
    addr <<= 2;
    addr += pc;

    bool cond_met = false;
    switch (GET_OP0(insn)) {
      case 0x0b:
        cond_met = (rt_val != ra_val);
        break;

      case 0x0c:
        cond_met = (rt_val > ra_val);
        break;
    }
    const uint32_t new_pc = cond_met ?  addr : (NEXT_PC(pc));
    TRY_SET_PC(new_pc, err);
    return true;
}

// LDUI (0x0d): Load the upper 16 bits of `rt` using `imm16` (`ra` is ignored).
static bool CuExecLoadUpImmOp(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    TRY_SET_IREG(GET_RT(insn), GET_IMM16(insn) << 16, err);
    TRY_SET_PC(NEXT_PC(pc), err);
    return true;
}

// LDWD (0x0e): Load a word into `rt` from memory at `ra` + sign_ext(`imm16`).
// LDHS (0x0f): Like LDWD, but for a sign-extended half-word.
// LDHU (0x10): Like LDHS, but for a half-word without sign-extension.
// LDBS (0x11): Like LDWD, but for a sign-extended single byte.
// LDBU (0x12): Like LDBS, but for a byte without sign-extension.
static bool CuExecLoadMemOps(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    uint32_t ra_val;
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);
    // Wrap-around semantics with over-/under-flow.
    const uint32_t addr = ra_val + GetSignExtImm16(insn);

    uint32_t rt_val;
    const uint8_t op0 = GET_OP0(insn);
    switch (op0) {
      case 0x0e: {
        if (!CuGetWordAt(addr, &rt_val, err)) {
            return false;
        }
        break;
      }

      case 0x0f:
      case 0x10: {
        uint16_t hw;
        if (!CuGetHalfWordAt(addr, &hw, err)) {
            return false;
        }
        rt_val = (uint32_t)hw;
        if (op0 == 0x0f && (hw & 0x8000U)) {
            rt_val |= 0xFFFF0000U;
        } else {
            rt_val &= 0x0000FFFFU;
        }
        break;
      }

      case 0x11:
      case 0x12: {
        uint8_t b;
        if (!CuGetByteAt(addr, &b, err)) {
            return false;
        }
        rt_val = (uint32_t)b;
        if (op0 == 0x11 && (b & 0x80U)) {
            rt_val |= 0xFFFFFF00U;
        } else {
            rt_val &= 0x000000FFU;
        }
        break;
      }
    }
    TRY_SET_IREG(GET_RT(insn), rt_val, err);
    TRY_SET_PC(NEXT_PC(pc), err);
    return true;
}

// STWD (0x13): Store the word in `rt` into memory at `ra` + sign_ext(`imm16`).
// STHW (0x14): Like STWD, but store a half-word (16 LSBs).
// STSB (0x15): Like STWD, but store a single byte (8 LSBs).
static bool CuExecStoreMemOps(uint32_t pc, uint32_t insn,
  CuError* restrict err) {
    uint32_t ra_val;
    TRY_GET_IREG(GET_RA(insn), &ra_val, err);
    // Wrap-around semantics with over-/under-flow.
    const uint32_t addr = ra_val + GetSignExtImm16(insn);

    uint32_t rt_val;
    TRY_GET_IREG(GET_RT(insn), &rt_val, err);

    switch (GET_OP0(insn)) {
      case 0x13:
        if (!CuSetWordAt(addr, rt_val, err)) {
            return false;
        }
        break;

      case 0x14:
        if (!CuSetHalfWordAt(addr, rt_val & 0x0000FFFFU, err)) {
            return false;
        }
        break;

      case 0x15:
        if (!CuSetByteAt(addr, rt_val & 0x000000FFU, err)) {
            return false;
        }
        break;
    }

    TRY_SET_PC(NEXT_PC(pc), err);
    return true;
}

void CuInitOps(void) {
    cup_op_executors[0x00] = CuExecOp0x00;

    cup_op_executors[0x01] = CuExecBoolImmOps;
    cup_op_executors[0x02] = CuExecBoolImmOps;
    cup_op_executors[0x03] = CuExecBoolImmOps;
    cup_op_executors[0x04] = CuExecAddImmOp;
    cup_op_executors[0x05] = CuExecJmpOps;
    cup_op_executors[0x06] = CuExecJmpOps;
    cup_op_executors[0x07] = CuExecFlagBranchOps;
    cup_op_executors[0x08] = CuExecFlagBranchOps;
    cup_op_executors[0x09] = CuExecFlagBranchOps;
    cup_op_executors[0x0a] = CuExecFlagBranchOps;
    cup_op_executors[0x0b] = CuExecCmpBranchOps;
    cup_op_executors[0x0c] = CuExecCmpBranchOps;
    cup_op_executors[0x0d] = CuExecLoadUpImmOp;
    cup_op_executors[0x0e] = CuExecLoadMemOps;
    cup_op_executors[0x0f] = CuExecLoadMemOps;
    cup_op_executors[0x10] = CuExecLoadMemOps;
    cup_op_executors[0x11] = CuExecLoadMemOps;
    cup_op_executors[0x12] = CuExecLoadMemOps;
    cup_op_executors[0x13] = CuExecStoreMemOps;
    cup_op_executors[0x14] = CuExecStoreMemOps;
    cup_op_executors[0x15] = CuExecStoreMemOps;

    // TODO: Define and implement the rest of the ISA.
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
