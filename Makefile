# Makefile

ROM_NAME = egg64

LIBDRAGON ?= /opt/libdragon
# The libdragon toolchain installs the cross-compilers in the top-level
# "bin" directory rather than under mips64-elf/.  Point the toolchain
# directory there so CC resolves to /opt/libdragon/bin/mips64-elf-gcc by
# default.
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

