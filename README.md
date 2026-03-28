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
5. When the ROM starts, it verifies that `egg.obj` loaded correctly. If the
   model fails to load, an error is printed and the program exits.

`egg64.z64` can be loaded in an emulator or on hardware.

## Development checks

Run these before opening a PR:

1. Standard build check:
   - `make clean && make`
2. Stricter warning check:
   - `make clean && make EXTRA_WARNINGS="-Wshadow -Wconversion"`

Notes about warnings/suppressions:

- The build deliberately uses `-Wno-error=deprecated-declarations` because
  some libdragon APIs currently used by this project are deprecated but still
  required for compatibility.
- libdragon headers are added with `-isystem` so strict warning checks focus on
  project code (`src/` and `include/`) instead of third-party SDK noise.

## License

This project is licensed under the terms of the [MIT License](LICENSE).
