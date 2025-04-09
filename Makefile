# Makefile

ROM_NAME = egg64

LIBDRAGON = /opt/libdragon

CC = mips64-elf-gcc
CFLAGS = -I$(LIBDRAGON)/mips64-elf/include -Iinclude -std=gnu99 -O2

LD = mips64-elf-ld
LDFLAGS = -L$(LIBDRAGON)/mips64-elf/lib -ldragon -ldragonsys -lc -lm -lgcc -lnosys

OBJCOPY = mips64-elf-objcopy
N64TOOL = n64tool

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

