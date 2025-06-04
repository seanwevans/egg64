# 🥚64

A Nintendo 64 homebrew project about an egg.

## Prerequisites

- A POSIX-compatible system (Linux or macOS recommended).
- The [libdragon](https://github.com/DragonMinded/libdragon) SDK installed in `/opt/libdragon`.
- The MIPS64 ELF toolchain and `n64tool` provided by libdragon.
- `make`.

## Building

1. Clone this repository.
2. Ensure the `LIBDRAGON` path in the `Makefile` points to your libdragon installation. Alternatively, run `make LIBDRAGON=/path/to/libdragon` to override the default.
3. Run `make`. The default target builds `egg64.z64`.
4. Use `make clean` to remove build artifacts.

`egg64.z64` can be loaded in an emulator or on hardware.

## License

This project is licensed under the terms of the [MIT License](LICENSE).
