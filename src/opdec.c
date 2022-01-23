// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "opdec.h"

#include <inttypes.h>
#include <stdio.h>

#define GET_OP0(insn) (uint8_t)(((insn & 0xFC000000U) >> 26) & 0x0000003FU)
#define GET_OP1(insn) (uint8_t)((insn) & 0x0000003FU)

#define GET_RT(insn) (uint8_t)(((insn & 0x03E00000U) >> 21) & 0x0000001FU)
#define GET_RA(insn) (uint8_t)(((insn & 0x001F0000U) >> 16) & 0x0000001FU)
#define GET_RB(insn) (uint8_t)(((insn & 0x0000F800U) >> 11) & 0x0000001FU)

#define GET_IMM5(insn) (uint8_t)(((insn & 0x000007C0U) >> 6) & 0x0000001FU)
#define GET_IMM16(insn) (insn & 0x0000FFFFU)
#define GET_IMM21(insn) (insn & 0x001FFFFFU)
#define GET_IMM26(insn) (insn & 0x03FFFFFFU)

static void CuDecodeOp0x00(uint32_t insn, char* restrict buf, size_t size) {
    const uint8_t rt = GET_RT(insn);
    const uint8_t ra = GET_RA(insn);
    const uint8_t rb = GET_RB(insn);
    const uint8_t imm5 = GET_IMM5(insn);

    switch (GET_OP1(insn)) {
      case 0x00:
        snprintf(buf, size, "SLLR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x01:
        snprintf(buf, size, "SLRF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x02:
        snprintf(buf, size, "SRLR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x03:
        snprintf(buf, size, "SRRF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x04:
        snprintf(buf, size, "SRAR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x05:
        snprintf(buf, size, "SRAS r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x06:
        snprintf(buf, size, "SLLI r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x07:
        snprintf(buf, size, "SLIF r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x08:
        snprintf(buf, size, "SRLI r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x09:
        snprintf(buf, size, "SRIF r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x0a:
        snprintf(buf, size, "SRAI r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x0b:
        snprintf(buf, size, "SRAJ r%d, r%d, %d", rt, ra, imm5);
        break;

      case 0x0c:
        snprintf(buf, size, "ANDR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x0d:
        snprintf(buf, size, "ADRF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x0e:
        snprintf(buf, size, "ORRR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x0f:
        snprintf(buf, size, "ORRF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x10:
        snprintf(buf, size, "NOTR r%d, r%d", rt, ra);
        break;

      case 0x11:
        snprintf(buf, size, "NOTF r%d, r%d", rt, ra);
        break;

      case 0x12:
        snprintf(buf, size, "XORR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x13:
        snprintf(buf, size, "XORF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x14:
        snprintf(buf, size, "ADDR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x15:
        snprintf(buf, size, "ADDF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x16:
        snprintf(buf, size, "SUBR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x17:
        snprintf(buf, size, "SUBF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x18:
        snprintf(buf, size, "MULR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x19:
        snprintf(buf, size, "MULF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x1a:
        snprintf(buf, size, "DIVR r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x1b:
        snprintf(buf, size, "DIVF r%d, r%d, r%d", rt, ra, rb);
        break;

      case 0x1c:
        snprintf(buf, size, "RDEP r%d", rt);
        break;

      case 0x1d:
        snprintf(buf, size, "WREP r%d", ra);
        break;

      case 0x1e:
        snprintf(buf, size, "JMPR r%d, r%d, %d", ra, rb, imm5);
        break;

      case 0x1f:
        snprintf(buf, size, "JALR r%d, r%d, %d", ra, rb, imm5);
        break;

      default:
        snprintf(buf, size, "????");
        break;
    }
}

void CuDecodeOp(uint32_t insn, char* restrict buf, size_t size) {
    switch (GET_OP0(insn)) {
      case 0x00:
        CuDecodeOp0x00(insn, buf, size);
        break;

      case 0x01:
        snprintf(buf, size, "ANDI r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x02:
        snprintf(buf, size, "ORRI r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x03:
        snprintf(buf, size, "XORI r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x04:
        snprintf(buf, size, "ADDI r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x05:
        snprintf(buf, size, "JMPI %08" PRIx32, GET_IMM26(insn));
        break;

      case 0x06:
        snprintf(buf, size, "JALI %08" PRIx32, GET_IMM26(insn));
        break;

      case 0x07:
        snprintf(buf, size, "BRNR r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x08:
        snprintf(buf, size, "BROR r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x09:
        snprintf(buf, size, "BRCR r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x0a:
        snprintf(buf, size, "BRZR r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x0b:
        snprintf(buf, size, "BRNE r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x0c:
        snprintf(buf, size, "BRGT r%d, %08" PRIx32, GET_RT(insn),
          GET_IMM21(insn));
        break;

      case 0x0d:
        snprintf(buf, size, "LDUI r%d, %04" PRIx32, GET_RT(insn),
          GET_IMM16(insn));
        break;

      case 0x0e:
        snprintf(buf, size, "LDWD r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x0f:
        snprintf(buf, size, "LDHS r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x10:
        snprintf(buf, size, "LDHU r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x11:
        snprintf(buf, size, "LDBS r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x12:
        snprintf(buf, size, "LDBU r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x13:
        snprintf(buf, size, "STWD r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x14:
        snprintf(buf, size, "STHW r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x15:
        snprintf(buf, size, "STSB r%d, r%d, %04" PRIx32, GET_RT(insn),
          GET_RA(insn), GET_IMM16(insn));
        break;

      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
      case 0x20:
      case 0x21:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
      case 0x28:
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2c:
      case 0x2d:
      case 0x2e:
      case 0x2f:
      case 0x30:
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
      case 0x38:
      case 0x39:
      case 0x3a:
      case 0x3b:
      case 0x3c:
      case 0x3d:
      case 0x3e:
      case 0x3f:
      default:
        snprintf(buf, size, "????");
        break;
    }
}
