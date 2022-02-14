# Makefile for CUSS.
.POSIX:

# Purge built-in suffix-based inference-rules to speed up builds.
.SUFFIXES:
.SUFFIXES: .c .o

# Include system-specific configuration for the build.
include config.mk

PKG = cuss
VER = 0.1.0

PRG = cuss

SRCS = \
       src/cpu.c \
       src/cuss.c \
       src/errors.c \
       src/logger.c \
       src/memory.c \
       src/monitor.c \
       src/opdec.c \
       src/ops.c \

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

all: $(PRG)

$(PRG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ $(LDLIBS)

install: $(PRG)
	$(MKDIR_P) $(DESTDIR)$(PREFIX)/bin
	$(CP_Q) $(PRG) $(DESTDIR)$(PREFIX)/bin
	@echo $(PKG)-$(VER) has been installed to $(DESTDIR)$(PREFIX).

uninstall:
	$(RM_Q) $(DESTDIR)$(PREFIX)/bin/$(PRG)
	-$(RMDIR) $(DESTDIR)$(PREFIX)/bin
	-$(RMDIR) $(DESTDIR)$(PREFIX)
	@echo $(PKG)-$(VER) has been uninstalled from $(DESTDIR)$(PREFIX).

clean:
	$(RM_Q) $(DEPS)
	$(RM_Q) $(OBJS)
	$(RM_Q) $(PRG)

depend: $(OBJS) $(DEPS)
	$(MK_DEPEND_MK)

# NOTE: Use `make depend` to update this file with auto-generated dependencies.
include depend.mk

.PHONY: all clean depend install uninstall
