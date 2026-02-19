# madi-nrf52

Firmware project targeting nRF52 (Cortex-M4F) with a CMake-first build and
optional Make-based workflow. This guide is focused on getting a new developer
productive quickly.

## Project Structure

```
.
├── CMakeLists.txt                 # Top-level CMake entry
├── include/                       # Public headers (version.h, metrics.def, etc.)
├── src/                           # Application sources
├── ports/                         # Platform-specific ports (nrf52, rtt, zephyr)
├── projects/                      # Build system modules (arch, platforms, warnings)
├── external/                      # Third-party sources (libmcu, SDKs)
├── tests/                         # Unit tests (Make-based)
└── Makefile                       # Legacy Make build (optional)
```

Key build files:
- `projects/arch/arm/*.cmake`: architecture options (common, cm3, cm4f)
- `projects/platforms/madi_nrf52840.cmake`: nRF52840 build + flash targets
- `projects/warnings.cmake`: warnings interface library
- `projects/external.cmake`: third-party library wiring (libmcu)

## Prerequisites

### Toolchain
Install the ARM GNU toolchain and ensure these binaries are on PATH:
- `arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-objcopy`

This repo expects the toolchain via `projects/arch/arm/arm-none-eabi-gcc.cmake`.

### Flashing Tools (nRF52)

Choose based on your setup:
- **J-Link / nRFJProg**: `nrfjprog` (used by `flash` and `flash_softdevice`)
- **USB DFU**: `dfu-util` (used by `flash_usb`)
- **GDB server**: `pyocd` (used by `gdb` target)

## Build (CMake)

From repo root:

```bash
cmake -S . -B build -DTARGET_PLATFORM=madi_nrf52840
cmake --build build
```

Build outputs:
- `build/madi.elf`
- `build/madi.hex`
- `build/madi.bin`
- `build/madi.map`

### Targets

```bash
# Build binary artifacts
cmake --build build --target madi.elf
cmake --build build --target madi.hex
cmake --build build --target madi.bin

# Flashing
cmake --build build --target flash          # nrfjprog .hex
cmake --build build --target flash_usb      # dfu-util .bin
cmake --build build --target flash_softdevice

# Debug (GDB server)
cmake --build build --target gdb            # pyocd gdbserver -t nrf52840
```

> Note: If you switch compiler flags or architecture options, clean the build
> directory (`rm -rf build`) to avoid stale objects.

## Build (Makefile - optional)

```bash
make
```

Make-based configuration lives under `projects/*.mk` and mirrors the CMake
workflow. Tests are also Make-based:

```bash
make test
make coverage
```

## Configuration

- Platform selection is controlled by `TARGET_PLATFORM`.
- For CMake: `-DTARGET_PLATFORM=madi_nrf52840`
- Some values are derived from `include/version.h` and `projects/version.cmake`.

## Common Issues

### ABI mismatch (VFP / Thumb)
If you see linker errors like:
```
uses VFP register arguments, ... does not
Unknown destination type (ARM/Thumb)
```
it indicates mixed architecture/ABI objects. Clean build and ensure all targets
inherit the same architecture options via the interface libraries.

---

If you’re new to the project, start with the CMake build flow above, then
flash via `flash` or `flash_usb` depending on your device setup.
