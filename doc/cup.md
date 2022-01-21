# CUP: Completely Useless Processor

## Introduction

The Completely Useless Processor (CUP) is a hypothetical CPU at the heart of
the [Completely Useless System](cus.md) (CUS). CUP is a simple 32-bit CPU
inspired by the
[RISC](https://en.wikipedia.org/wiki/Reduced_instruction_set_computer)
design-principles. It aims to provide a simple and regular
[ISA](https://en.wikipedia.org/wiki/Instruction_set_architecture) with a set of
reduced instructions (and *not* a reduced set of instructions) for the most
common tasks.

A 32-bit CPU happens to be more than enough for my educational-needs, so CUP is
not a 64-bit CPU like modern CPUs. This makes it much easier to efficiently
simulate it on older hardware. CUP is designed to represent a modest circa 2000s
"desktop-class" CPU. It does *not* aim to serve memory-constrained embedded
devices or "big-iron" server-class hardware. CUP therefore uses non-compact
32-bit instructions and can only address up to 4 GiB of physical RAM.

The native word-size for CUP is 32 bits. It allows programs to read and write
from unaligned memory-locations, albeit with a slight performance-penalty. It
also allows directly reading and writing half-words (16 bits) and bytes (8 bits)
for convenience.

## Registers

CUP provides access to 32 general-purpose 32-bit integer registers. These are
numbered from `r0` to `r31`. Of these 32 registers, `r0` is hard-wired to zero
(writes to it are ignored and reads from it always return `0`) and `r31` is
the link-register used to pass the return-address during procedure-calls. The
calling-conventions for a programming-language would typically impose further
restrictions on how the remaining 30 registers should be used.

![Registers In CUP](regs.png)

For temporarily holding a 64-bit extended-precision value during multiplication
or division, there is an additional `ep` register to hold the upper 32 bits of a
product or dividend respectively.

CUP has a 32-bit program-counter register `pc` that points to the next
instruction to be executed by the CPU. There are no instructions to directly
access `pc`.

CUP also has a 32-bit `flags` register to represent various bits of information
about the program-state. As of now, only four flags are defined - `N`
(negative), `O` (overflow), `C` (carry), and `Z` (zero) - to represent various
conditions for integer arithmetic.

Note that the integer condition-code flags are usually only set by some
arithmetic (and some logical) instructions that explicitly request them to be
set. They are otherwise not set, in order to reduce implicit inter-instruction
data-dependencies.

## Instructions

Instructions for CUP are fixed-length 32-bit words. They must be word-aligned
in memory (thus the least significant two bits of the `pc` register must always
be zero).

### Encoding

The following diagram shows the instruction-formats used by CUP:

![Instruction-Formats For CUP](ifmts.png)

As can be seen from the diagram, each instruction-format encodes fields with
similar meanings at the same positions. For example:

*   The 6 bits in positions 31-26 form the `op0` field common to *all*
formats and contain the primary opcode for the instruction.
*   The 5 bits in positions 25-21 form the `rt` field for formats that use it
to represent the target register of an operation.
*   The 5 bits in positions 20-16 form the `ra` field for formats that use it
to represent the primary source operand register of an operation.

The instruction-formats are:

1.  R-type, for instructions with a target register operand and two source
register operands (or that need a small immediate operand, like some logical
shift instructions).
1.  I-type, for instructions with a target register operand, a source register
operand, and a sign-extended 16-bit immediate operand (zero-extended in case of
logical instructions).
1.  B-type, for instructions that use a single register operand and a 21-bit
sign-extended immediate operand (like conditional branch instructions).
1.  J-type, for instructions that need to use all the available space for a
sign-extended 26-bit immediate operand (like unconditional jumps).

The uniform encoding of instructions make it relatively easy to decode them for
execution.

### Conventions

The mnemonics for the instructions in this document are always 4 uppercase
letters (32-bit mnemonics for 32-bit instructions). The target of an instruction
comes before its source operands.

### Logical Instructions

TODO: Flesh this out.

### Arithmetic Instructions

TODO: Flesh this out.

### Control-Flow Instructions

TODO: Flesh this out.

### Load-Store Instructions

TODO: Flesh this out.

## TODOs

To be a bit more realistic, CUP still needs support for a great many things:

*   How to interact with hardware peripherals for input/output.
*   Raising and handling traps.
*   Calling-convention for C programs.
*   User versus supervisor mode and privileged instructions.
*   Memory-management unit and support for virtual memory.
*   Floating-point arithmetic.
*   Simulation of a memory-hierarchy, including appropriate latency-hits.
*   Multi-core support, including having a well-defined memory-model.
*   Pipelined execution of instructions.
*   (Nice to have) Vector instructions for multimedia.
*   Et cetera.

I plan to add support for these in due course of time.
