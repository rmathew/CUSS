# Where to install the software.
PREFIX = /usr/local

# How to find the SDL2 headers.
# TIP: Use `sdl2-config --cflags` or `pkg-config sdl2 --cflags` to get the
# correct values for your system.
SDL2_INCS = -I/usr/include/SDL2 -D_REENTRANT

# How to link against the SDL2 libraries.
# TIP: Use `sdl2-config --libs` or `pkg-config sdl2 --libs` to get the
# correct values for your system.
SDL2_LIBS = -L/usr/lib -Wl,-rpath,/usr/lib -Wl,--enable-new-dtags -lSDL2

# Which compiler and linker-wrapper to use.
CC = cc

# Which checks to perform on the source-code.
WRN_FLAGS = -std=c99 -Wall -Wextra -Wpedantic

# How much to optimize the generated code.
OPT_FLAGS = -DNDEBUG -O3

# How to instruct the compiler to generate a make-compliant ".d" dependency-
# file as the side-effect of compiling a ".c" file.
#
# WARNING: This only works with GCC and compilers that emulate its flags.
MAK_FLAGS = -MT $(<:.c=.o) -MMD -MP

# Additional flags per source-file useful during development.
DEV_FLAGS = $(MAK_FLAGS) -g1 -Werror -pedantic-errors

# Which flags to pass to the C compiler.
CFLAGS = $(INCS) $(WRN_FLAGS) $(OPT_FLAGS)

# Which flags to pass to the linker-wrapper.
LDFLAGS =

# Which extra libraries to include during the linking step.
LDLIBS =

# Gather the automatically-generated dependency-information into a single file.
MK_DEPEND_MK = sort -u $(DEPS) | sed 's/^[ \t]*$$//' | sed '/^$$/d' > depend.mk

# How to create an object-file from source-code.
.c.o:
	$(CC) $(CFLAGS) $(DEV_FLAGS) -c $< -o $@

# How to remove a file without complaining about missing files.
RM_Q = rm -f

# How to copy a file without complaining about existing files.
CP_Q = cp -f

# How to create a directory and intermediate directories as needed.
MKDIR_P = mkdir -p

# How to remove an empty directory.
RMDIR = rmdir
