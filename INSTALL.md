# Installing CUSS

## Requirements

You will need at least the following to build CUSS:

*   A C compiler that supports the C99 standard.

*   A POSIX-compliant implementation of the `make` tool.

*   The [SDL2](https://www.libsdl.org/) library.

I have tested CUSS on an x86-32 Linux machine with GCC v9.4.0, GNU Make v4.3,
and SDL2 v2.0.16. (I have also tested the build with pmake v1.111 and bmake
v20211221 on the same machine.)

## Build And Installation

Edit the `config.mk` file as needed for your system and then run
`make PREFIX=/foo/bar` to install CUSS into the directory `/foo/bar`.
