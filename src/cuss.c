// SPDX-FileCopyrightText: Copyright (c) 2022 Ranjit Mathew.
// SPDX-License-Identifier: BSD-3-Clause
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"
#include "memory.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ERROR: Invalid number of arguments.\n");
        fprintf(stderr, "Usage: %s <mem-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    CuError err;
    if (!CussInitMemFromFile(argv[1], &err)) {
        fprintf(stderr, "ERROR: Could not process '%s': %s\n", argv[1],
          err.err_msg);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
