# CUSS: Completely Useless System Simulator

## Introduction

See the INSTALL.md file for instructions on building and installing CUSS.

CUSS is licensed under the terms of the three-clause BSD license. See the
LICENSE file for more details.

## Memory-Image

A memory-image is a simple file containing a series of sections to be loaded
into the respective memory-address. Each such section starts with a
base-address, followed by the number of bytes to load starting that
base-address, followed by the actual bytes themselves. Both the base-address and
the number of bytes are unsigned 32-bit numbers, each encoded using four
little-endian bytes.

You can use your favorite hex-editor to create such a memory-image file. For
example, using the `xxd` tool available with Vim:

```shell
$ cat test.txt
00: 00 00 00 00 08 00 00 00 de ad be ef ca fe ba be
10: 00 01 00 00 04 00 00 00 c0 de 65 02

$ xxd -r test.txt test.mem
```

This creates a memory-image to load the given eight bytes at the memory-address
`0x00000000` and another four bytes at `0x00000100`.
