# CUSS: Completely Useless System Simulator

## Introduction

CUSS is a work-in-progress simulator for the Completely Useless System (CUS),
which is based on the Completely Useless Processor (CUP).

See the [INSTALL.md](INSTALL.md) file for instructions on building and
installing CUSS. CUSS is licensed under the terms of the three-clause BSD
license. See the [LICENSE](LICENSE) file for more details.

## Running CUSS

At the moment, CUSS is a possibly-buggy and definitely-slow interpreter for a
subset of the CUP ISA, let alone the other components of CUS. Since CUSS is a
bare-bones interpreter, you need to first initialize the contents of its
simulated RAM with some suitable code and data, before you can see it in action.
This is done via a memory-image file.

To run CUSS with an initial memory-state defined by the memory-image file named
`foo.mem`, execute:

```shell
cuss foo.mem
```

You can then step through the instructions as they are executed and see the
contents of the various registers. Note that the [reset
vector](https://en.wikipedia.org/wiki/Reset_vector) for CUP is `0x00000000`, so
every memory-image *must* provide some code at that location.

### Memory-Image

A memory-image is a simple file containing a series of sections containing data
to be loaded into the respective memory-addresses. Each such section starts with
a base-address, followed by the number of bytes to load into memory starting at
that base-address, followed by the actual bytes to be loaded. Both the
base-address and the number of bytes are unsigned 32-bit numbers, each encoded
using four little-endian bytes.

You can use your favorite hex-editor to create such a memory-image file. For
example, you can use the [xxd](https://github.com/ConorOG/xxd/) tool by Juergen
Weigert (which is also available with [Vim](https://www.vim.org/)). You can
combine it with other Unix utilities, say, to provide rudimentary support for
single-line comments starting with a `#` character:

```shell
cut -f1 -d'#' test.txt | xxd -p -r - test.mem
```

If you are comfortable working with hexadecimal numbers as well as writing *raw*
machine-code for CUP (who isn't?), you can write programs using this method and
feed it to CUSS.

As an example, here is a simple program that can be converted into a
memory-image for CUSS using `xxd` and `cut` as shown above:
```shell
00 00 00 00    # At the address 0x00000000...
24 00 00 00    # ...load the following 36 bytes.
#
00 01 20 38    # LDWD r1, r0, 0x100  (r1 := seed)
09 00 40 08    # ORRI r2, r0, 9  (r2 := 9)
87 00 21 00    # SLLI r1, r1, 2  (r1 := r1 * 4)
01 00 21 10    # ADDI r1, r1, 1  (r1 := r1 + 1)
1d 00 00 00    # WREP r0  (ep := 0)
1a 10 01 00    # DIVR r0, r1, r2  (ep:r1 / r2)
1c 00 20 00    # RDEP r1  (r1 := r1 % r2)
0e 00 61 00    # ORRR r3, r1, r0  (r3 := new random-number)
fa ff ff 17    # JMPI -6 (jump back six words, forming a loop)

00 01 00 00    # At the address 0x00000100...
04 00 00 00    # ...load the following 4 bytes.
#
2f cb 04 00    # The number 314159 as the seed.
```

This little program is a [linear congruential
generator](https://en.wikipedia.org/wiki/Linear_congruential_generator) of
pseudo-random numbers in the range `0..8` (watch register `r3` for each such
pseudo-random number), using the number stored at memory-address `0x00000100`
as a seed.
