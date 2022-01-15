# CUSS: Completely Useless System Simulator

## Introduction

CUSS is a simulator for the Completely Useless System (CUS) based on the
Completely Useless Processor (CUP).

To run CUSS after initializing the contents of its memory using a memory-image
file named `foo.mem`, execute:

```shell
$ cuss foo.mem
```

You can then step through the instructions as they are executed (of course,
after instruction-execution is actually implemented) and see the contents of
the various registers.

See the [INSTALL.md](INSTALL.md) file for instructions on building and
installing CUSS.

CUSS is licensed under the terms of the three-clause BSD license. See the
[LICENSE](LICENSE) file for more details.

## Memory-Image

A memory-image is a simple file containing a series of sections to be loaded
into the respective memory-addresses. Each such section starts with a
base-address, followed by the number of bytes to load into memroy starting at
that base-address, followed by the actual bytes to be loaded. Both the
base-address and the number of bytes are unsigned 32-bit numbers, each encoded
using four little-endian bytes.

You can use your favorite hex-editor to create such a memory-image file. For
example, using the `xxd` tool that is available with Vim:

```shell
$ cat test.txt
00: 00 00 00 00 08 00 00 00 de ad be ef ca fe ba be
10: 00 01 00 00 04 00 00 00 c0 de 65 02

$ xxd -r test.txt test.mem
```

This creates a memory-image file `test.mem` that tells CUSS to load the given
eight bytes (`de ad be ef ca fe ba be` in hexadecimal) at the memory-address
`0x00000000` and another four bytes (`c0 de 65 02`) at the memory-address
`0x00000100`.

WARNING: These are *not* valid instructions for CUP at the moment. The example
above is provided purely for illustrative purposes.
