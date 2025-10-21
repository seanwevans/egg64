# Makefile

ROM_NAME = egg64

LIBDRAGON ?= /opt/libdragon

# Support both the modern libdragon layout (toolchain in $(LIBDRAGON)/bin)
# and the older layout (toolchain in $(LIBDRAGON)/mips64-elf/bin).  When the
# newer location is missing but the legacy one exists, prefer the legacy
# directory.  Users can still override TOOLCHAIN explicitly when invoking
# make.
ifeq ($(wildcard $(LIBDRAGON)/bin/mips64-elf-gcc),)
ifneq ($(wildcard $(LIBDRAGON)/mips64-elf/bin/mips64-elf-gcc),)
TOOLCHAIN ?= $(LIBDRAGON)/mips64-elf/bin
endif
endif
TOOLCHAIN ?= $(LIBDRAGON)/bin
MIPS_PREFIX ?= $(TOOLCHAIN)/mips64-elf-

CC = $(MIPS_PREFIX)gcc
CFLAGS = -I$(LIBDRAGON)/mips64-elf/include -Iinclude -std=gnu99 -O2 -Wall -Wextra

LD = $(MIPS_PREFIX)ld
LDFLAGS = -L$(LIBDRAGON)/mips64-elf/lib -ldragon -ldragonsys -lc -lm -lgcc -lnosys

OBJCOPY = $(MIPS_PREFIX)objcopy
N64TOOL ?= $(LIBDRAGON)/bin/n64tool

OBJS = build/main.o build/model_loader.o

all: $(ROM_NAME).z64

$(ROM_NAME).z64: $(OBJS)
	$(CC) -T n64.ld -o build/$(ROM_NAME).elf $(OBJS) $(LDFLAGS)	
	$(N64TOOL) -l 16M -t "EGG64" -o $(ROM_NAME).z64 build/$(ROM_NAME).elf -s 4M assets

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

clean:
	rm -rf build *.bin *.z64

-include $(OBJS:.o=.d)

