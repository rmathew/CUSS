// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include "memory.h"

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CUSS_MEMSIZE (1 << 20)

static uint8_t cuss_mem[CUSS_MEMSIZE];

static inline uint32_t LeQuadBytesToUint32(const uint8_t* bytes) {
    return (uint32_t)(bytes[0]) | ((uint32_t)(bytes[1]) << 8) |
      ((uint32_t)(bytes[2]) << 16) | ((uint32_t)(bytes[3]) << 24);
}

bool CuIsValidPhyMemAddr(uint32_t addr, CuError* restrict err) {
    if (addr >= CUSS_MEMSIZE) {
        return CuErrMsg(err, "Bad memory-address (0x%08" PRIx32 ").", addr);
    }
    return true;
}

bool CuByteAt(uint32_t addr, uint8_t* restrict val, CuError* restrict err) {
    if (!CuIsValidPhyMemAddr(addr, err)) {
        return false;
    }
    if (val == NULL) {
        return CuErrMsg(err, "NULL fetch-location.");
    }
    // TODO: Maybe check for unaligned memory-access.
    *val = cuss_mem[addr];
    return true;
}

bool CuWordAt(uint32_t addr, uint32_t* restrict val, CuError* restrict err) {
    if (!CuIsValidPhyMemAddr(addr, err)) {
        return false;
    }
    if (val == NULL) {
        return CuErrMsg(err, "NULL fetch-location.");
    }
    // TODO: Maybe check for unaligned memory-access.
    *val = LeQuadBytesToUint32(cuss_mem + addr);
    return true;
}

bool CuInitMemFromFile(const char* restrict file, CuError* restrict err) {
    if (file == NULL) {
        return CuErrMsg(err, "Missing file-name.");
    }

    FILE *f = fopen(file, "rb");
    if (f == NULL) {
        return CuErrMsg(err, "Could not open file (%s).", strerror(errno));
    }
    char buffer[BUFSIZ];
    if (setvbuf(f, buffer, _IOFBF, BUFSIZ)) {
        fclose(f);
        return CuErrMsg(err, "Could not set input-buffer.");
    }

    do {
        uint8_t header[8];
        size_t nhdr = fread(header, 1, sizeof header, f);
        if (nhdr == 0) {
            fclose(f);
            return true;
        }
        if (nhdr < sizeof header) {
            fclose(f);
            return CuErrMsg(err, "Truncated section-header (%u < %u).", nhdr,
              sizeof header);
        }

        uint32_t base = LeQuadBytesToUint32(header);
        uint32_t nbytes = LeQuadBytesToUint32(header + 4);

        // TODO: Handle overflows.
        if (!CuIsValidPhyMemAddr(base + nbytes, err)) {
            return CuErrMsg(err,
              "Out of bounds (base=0x%08" PRIx32 " + nbytes=0x%08" PRIx32
              " > 0x%08" PRIx32 ").", base, nbytes, CUSS_MEMSIZE);
        }
        size_t ndata = fread(cuss_mem + base, 1, nbytes, f);
        if (ndata < nbytes) {
            fclose(f);
            return CuErrMsg(err, "Truncated section-data (%u < %u).", ndata,
              nbytes);
        }
        printf("INFO: Loaded nbytes=0x%08" PRIx32 " at base=0x%08" PRIx32 "\n",
          nbytes, base);
    } while (!feof(f) && !ferror(f));
    if (ferror(f)) {
        fclose(f);
        return CuErrMsg(err, "Error reading file.");
    }

    fclose(f);
    return true;
}
