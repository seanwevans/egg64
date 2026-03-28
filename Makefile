ROM_NAME = egg64

LIBDRAGON ?= /opt/libdragon

# Detect modern vs legacy libdragon toolchain layout.
#
# Newer libdragon releases install the cross compiler, libraries and headers
# directly under $(LIBDRAGON)/bin, $(LIBDRAGON)/lib and $(LIBDRAGON)/include.
# Older releases placed the toolchain under a mips64-elf subfolder.
#
# Prefer the new layout if it exists, but fall back to the legacy one if the
# compiler binary cannot be found in $(LIBDRAGON)/bin.  Users can always set
# TOOLCHAIN and MIPS_PREFIX manually on the make command line.
ifneq ($(wildcard $(LIBDRAGON)/bin/mips64-elf-gcc),)
TOOLCHAIN ?= $(LIBDRAGON)/bin
else ifneq ($(wildcard $(LIBDRAGON)/mips64-elf/bin/mips64-elf-gcc),)
TOOLCHAIN ?= $(LIBDRAGON)/mips64-elf/bin
endif
MIPS_PREFIX ?= $(TOOLCHAIN)/mips64-elf-

CC  = $(MIPS_PREFIX)gcc
LD  = $(MIPS_PREFIX)gcc

# Use the top-level include directory with modern libdragon.
#
# We include libdragon through -isystem so warnings coming from SDK headers do
# not drown out warnings from project code when running stricter CI checks.
#
# Deliberate suppression: keep deprecated libdragon APIs as non-fatal because
# some currently-used APIs (for example controller_scan) are intentionally
# deprecated but still required for compatibility.
BASE_CFLAGS = -isystem $(LIBDRAGON)/include -Iinclude -std=gnu99 -O2 -Wall \
    -Wextra -Wno-error=deprecated-declarations

# Optional warning flags used by stricter local/CI checks.
EXTRA_WARNINGS ?=

CFLAGS = $(BASE_CFLAGS) $(EXTRA_WARNINGS)

# Link against libdragon.  The modern layout installs libraries under
# $(LIBDRAGON)/lib; the linker will search the appropriate libgcc/nostd
# directories automatically.
LDFLAGS = -L$(LIBDRAGON)/lib -ldragon -ldragonsys -lc -lm -lgcc -lnosys

OBJS = build/main.o build/model_loader.o

# Use the n64tool included with libdragon.  A custom value can be passed on
# the command line if needed.
N64TOOL ?= $(LIBDRAGON)/bin/n64tool

all: $(ROM_NAME).z64

# Build the ELF using the cross compiler, then package it into a ROM.
# Modern n64tool requires that the ROM size be specified with -l, and
# additional files (directories) are appended without specifying -s.  The
# title is set via -t.  Assets are bundled to create a rompak that
# includes our DragonFS image.  See libdragon documentation for details.
$(ROM_NAME).z64: $(OBJS)
	$(LD) -T n64.ld -o build/$(ROM_NAME).elf $(OBJS) $(LDFLAGS)
	$(N64TOOL) -l 16M -t "EGG64" -o $(ROM_NAME).z64 build/$(ROM_NAME).elf assets

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

clean:
	rm -rf build *.bin *.z64

-include $(OBJS:.o=.d)
