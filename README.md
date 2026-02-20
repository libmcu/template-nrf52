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

## Zephyr OS (Freestanding)

This project supports building with Zephyr OS in **freestanding mode** — no
west workspace is required inside the repo. The Zephyr installation lives
outside the project directory.

### Prerequisites

#### 1. Zephyr SDK / Zephyr source

Install Zephyr 3.7 (or later) somewhere outside this repo, e.g.:

```
~/Work/c/zephyr/
├── zephyr/          # Zephyr kernel source (ZEPHYR_BASE)
├── modules/         # hal_nordic, mbedtls, littlefs, segger …
└── bootloader/      # mcuboot
```

Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) to set up the workspace (west init / west update).

#### 2. ARM GNU Toolchain

Download the **arm-none-eabi** toolchain (GCC 12+) and extract it, e.g.:

```
~/.local/gcc-arm-none-eabi/
├── bin/
│   ├── arm-none-eabi-gcc
│   └── …
```

Official download: <https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads>

#### 3. Ninja

```bash
brew install ninja          # macOS
sudo apt install ninja-build  # Ubuntu/Debian
```

#### 4. Flash Tools

| Tool | Use |
|------|-----|
| `nrfjprog` | J-Link / nRF5 programming (recommended) |
| `pyocd` | Alternative OpenOCD-based flashing |

### Environment Variables

Set these in your shell (or add to `.bashrc` / `.zshrc`):

```bash
export ZEPHYR_BASE=~/Work/c/zephyr/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=~/.local/gcc-arm-none-eabi
```

Adjust paths to match your actual installation locations.

### Build (Zephyr)

```bash
# Configure (first time or after Kconfig/DTS changes)
cmake -B build/madi_nrf52840_zephyr \
      -DTARGET_PLATFORM=madi_nrf52840 \
      -G Ninja

# Build
cmake --build build/madi_nrf52840_zephyr
```

Or in a single one-liner with inline env vars:

```bash
ZEPHYR_BASE=~/Work/c/zephyr/zephyr \
ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb \
GNUARMEMB_TOOLCHAIN_PATH=~/.local/gcc-arm-none-eabi \
cmake -B build/madi_nrf52840_zephyr -DTARGET_PLATFORM=madi_nrf52840 -G Ninja && \
cmake --build build/madi_nrf52840_zephyr
```

Build outputs in `build/madi_nrf52840_zephyr/zephyr/`:
- `madi.elf` — debug ELF
- `madi.hex` — Intel HEX for flashing
- `madi.bin` — raw binary

### Flash (Zephyr)

#### J-Link / nrfjprog

```bash
# Erase + program + reset
nrfjprog --family NRF52 --eraseall
nrfjprog --family NRF52 --program build/madi_nrf52840_zephyr/zephyr/madi.hex --verify
nrfjprog --family NRF52 --reset
```

#### west (if you have a west workspace)

```bash
west flash --build-dir build/madi_nrf52840_zephyr
```

#### pyocd

```bash
pyocd flash -t nrf52840 build/madi_nrf52840_zephyr/zephyr/madi.hex
```

### Zephyr-specific Notes

- **Board name**: `madi_nrf52840` (defined in `ports/zephyr/boards/nordic/madi_nrf52840/`)
- **Kconfig**: `ports/zephyr/prj.conf`
- **UART console**: TX = P0.23, RX = P0.25 (115200 baud)
- **LED**: P0.20 (active-low, alias `led0`)
- The `ZEPHYR_TOOLCHAIN_VARIANT` environment variable is what switches the
  build into Zephyr mode. If it is unset, CMake falls back to the nRF5 SDK
  standalone build.

---

If you're new to the project, start with the CMake build flow above, then
flash via `flash` or `flash_usb` depending on your device setup.
