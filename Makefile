ROM_NAME = egg64

LIBDRAGON ?= /opt/libdragon

ifneq ($(wildcard $(LIBDRAGON)/bin/mips64-elf-gcc),)
TOOLCHAIN ?= $(LIBDRAGON)/bin
else ifneq ($(wildcard $(LIBDRAGON)/mips64-elf/bin/mips64-elf-gcc),)
TOOLCHAIN ?= $(LIBDRAGON)/mips64-elf/bin
endif
MIPS_PREFIX ?= $(TOOLCHAIN)/mips64-elf-

CC  = $(MIPS_PREFIX)gcc
LD  = $(MIPS_PREFIX)gcc

BASE_CFLAGS = -isystem $(LIBDRAGON)/include -Iinclude -std=gnu99 -O2 -Wall \
    -Wextra -Wno-error=deprecated-declarations

EXTRA_WARNINGS ?=

CFLAGS = $(BASE_CFLAGS) $(EXTRA_WARNINGS)
LDFLAGS = -L$(LIBDRAGON)/mips64-elf/lib -ldragon -ldragonsys -lc -lm -lgcc -lnosys

OBJS = build/main.o build/model_loader.o

N64TOOL ?= $(LIBDRAGON)/bin/n64tool

all: $(ROM_NAME).z64

$(ROM_NAME).z64: $(OBJS)
	$(LD) -T n64.ld -o build/$(ROM_NAME).elf $(OBJS) $(LDFLAGS)
	$(N64TOOL) -l 16M -t "EGG64" -o $(ROM_NAME).z64 build/$(ROM_NAME).elf assets

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

clean:
	rm -rf build *.bin *.z64

-include $(OBJS:.o=.d)
