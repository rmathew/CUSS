// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "errors.h"
#include "memory.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ERROR: Invalid number of arguments.\n");
        fprintf(stderr, "Usage: %s <memory-image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    CuError err;
    printf("INFO: Loading memory-image from file '%s'...\n", argv[1]);
    if (!CussInitMemFromFile(argv[1], &err)) {
        fprintf(stderr, "ERROR: Could not load '%s': %s\n", argv[1],
          err.err_msg);
        return EXIT_FAILURE;
    }

    CussInitCpu();
    do {
        putchar('\n');
        if (!CussPrintCpuState(&err)) {
            fprintf(stderr, "ERROR: Could not print CPU-state: %s\n",
              err.err_msg);
            return EXIT_FAILURE;
        }
        printf("Press ENTER to continue...");
        fflush(stdout);
        getchar();

        if (!CussExecNextInsn(&err)) {
            fprintf(stderr, "ERROR: Could not execute instruction: %s\n",
              err.err_msg);
            return EXIT_FAILURE;
        }
    } while (true);

    return EXIT_SUCCESS;
}
