# CUS: Completely Useless System

## Introduction

The Completely Useless System (CUS) is a hypothetical computer built around the
equally-hypothetical [Completely Useless Processor](cup.md) (CUP) CPU. It is a
back-to-basics hobby-project that I started with the primary goal of relearning
some of the basics of systems programming and computer architecture that are
useful as a computer programmer.

## Hardware

Since this is a self-education project and since I am *not* a hardware engineer,
the hypothetical hardware-peripherals surrounding CUP are somewhat-idealized
abstractions of real-world hardware. Hardware in the real world (and standards
to interact with them) are usually products of historical accidents, industry
politics, business deals among companies, etc. I am not interested in all that,
not at the moment anyway, so I am eliding all the complexities of having to
simulate such hardware.

So CUS would have the following simulated peripherals to work with:

*   `M` MiB of RAM (`M` is `1` by default, but is configurable).
*   `N` MB of disk-space (depends on the size of the input disk-image).
*   a display-adapter with 1024x768 pixels at 32-bpp (no 3D-acceleration).
*   a 104-key US English keyboard.
*   a three-button mouse with a scroll-wheel.

Yes, this is ridiculously-anemic, even by the standards of desktops twenty years
ago, but this is all I need for my purposes at the moment. It also helps to keep
the system-requirements of the simulator quite modest as an added bonus.

**NOTE:** In the current implementation of the simulator, only the RAM is
simulated. I will slowly add support for the other peripherals over time.
