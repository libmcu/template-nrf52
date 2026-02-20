# madi-nrf52

Firmware for MADI nRF52840 (Cortex-M4F). Zephyr-based with MCUboot bootloader,
external QSPI NOR flash for OTA updates, and automatic rollback on boot failure.

## Project Structure

```
.
├── CMakeLists.txt                 # Top-level entry (Zephyr or nRF5 SDK)
├── include/                       # Public headers (version.h, metrics.def …)
├── src/                           # Application sources
├── ports/
│   └── zephyr/
│       ├── boards/nordic/madi_nrf52840/  # Board definition (DTS, Kconfig)
│       ├── prj.conf               # Application Kconfig
│       └── mcuboot.conf           # MCUboot Kconfig overlay
├── projects/
│   └── platforms/
│       └── zephyr.cmake           # Zephyr build wiring (signing key, modules)
├── secrets/
│   └── dfu_signing_dev.key        # Dev signing key — never commit to VCS
└── external/                      # libmcu and other third-party sources
```

Key files:

| File | Purpose |
|------|---------|
| `ports/zephyr/boards/nordic/madi_nrf52840/madi_nrf52840.dts` | Flash partitions, QSPI, GPIO |
| `ports/zephyr/prj.conf` | Application Kconfig |
| `ports/zephyr/mcuboot.conf` | MCUboot Kconfig (signature algorithm, swap, QSPI) |
| `projects/platforms/zephyr.cmake` | Signing key path, Zephyr module list |

---

## Prerequisites

### 1. Zephyr workspace

Install the Zephyr SDK outside this repo using
[west](https://docs.zephyrproject.org/latest/develop/getting_started/index.html):

```bash
west init ~/Work/c/zephyr
cd ~/Work/c/zephyr
west update
```

Expected layout:

```
~/Work/c/zephyr/
├── zephyr/          # Zephyr kernel ($ZEPHYR_BASE)
├── modules/         # hal_nordic, mbedtls, littlefs, segger …
└── bootloader/      # mcuboot
```

### 2. ARM GNU Toolchain (GCC 12+)

Download **arm-none-eabi** from
[developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
and extract, e.g. to `~/.local/gcc-arm-none-eabi/`.

### 3. Build tools

```bash
brew install ninja              # macOS
sudo apt install ninja-build    # Ubuntu / Debian
```

### 4. Flash tools

| Tool | Use |
|------|-----|
| `nrfjprog` | J-Link / nRF5 — initial flash and app updates |
| `mcumgr` | OTA / DFU over serial or BLE |

Install mcumgr:

```bash
pip install mcumgr
```

### 5. imgtool

```bash
pip install -r $ZEPHYR_BASE/../bootloader/mcuboot/scripts/requirements.txt
```

---

## Environment Setup

Add to your shell profile (`.bashrc` / `.zshrc`) and reload:

```bash
python3 -m venv ./.venv
source ./.venv/bin/activate
export PATH=.venv/bin:$PATH

export ZEPHYR_BASE=~/Work/c/zephyr/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=~/.local/gcc-arm-none-eabi
```

---

## Initial Setup

Build MCUboot and the app, then flash everything to a blank device.

### 1. Build MCUboot

```bash
west build -b madi_nrf52840 -d build/mcuboot \
    $ZEPHYR_BASE/../bootloader/mcuboot/boot/zephyr \
    -- -DBOARD_ROOT=$(pwd)/ports/zephyr \
    "-DEXTRA_CONF_FILE=$(pwd)/ports/zephyr/mcuboot.conf" \
    "-DCONFIG_BOOT_SIGNATURE_KEY_FILE=\"$(pwd)/secrets/dfu_signing_dev.key\""
```

Output: `build/mcuboot/zephyr/zephyr.hex`

### 2. Build App

```bash
west build -b madi_nrf52840 -d build/app

or

west build -b madi_nrf52840 -d build/app \
    -- -DBOARD_ROOT=$(pwd)/ports/zephyr
```

Signed outputs in `build/app/zephyr/`:

| File | Use |
|------|-----|
| `zephyr.signed.hex` | Test image — reverts on next reset if not confirmed |
| `zephyr.signed.bin` | Signed binary for OTA upload |
| `zephyr.signed.confirmed.hex` | Pre-confirmed — no runtime confirmation needed |

### 3. Flash (first-time)

```bash
# Erase chip
nrfjprog --family NRF52 --eraseall

# Flash MCUboot
west flash -d build/mcuboot

# Flash confirmed app to slot0 (--sectorerase preserves MCUboot at 0x0)
nrfjprog --family NRF52 \
    --program build/app/zephyr/zephyr.signed.confirmed.hex \
    --verify --sectorerase

nrfjprog --family NRF52 --reset
```

---

## Development Workflow

MCUboot stays on the device. Only rebuild and reflash the app.

### Build App

```bash
west build -d build/app
```

Board and configuration are cached; `-b` and `--` are not needed again.

### Flash App

```bash
nrfjprog --family NRF52 \
    --program build/app/zephyr/zephyr.signed.confirmed.hex \
    --verify --sectorerase
nrfjprog --family NRF52 --reset
```

### OTA / DFU

Upload the signed image to slot 1 and trigger a swap on next boot:

```bash
# Upload image to slot 1
mcumgr --conntype serial --connstring /dev/ttyUSB0,baud=115200 \
    image upload build/app/zephyr/zephyr.signed.bin

# List images — copy the hash of the uploaded image
mcumgr --conntype serial --connstring /dev/ttyUSB0,baud=115200 \
    image list

# Mark as pending (test mode — reverts if not confirmed before next reset)
mcumgr --conntype serial --connstring /dev/ttyUSB0,baud=115200 \
    image test <hash>

# Trigger swap
mcumgr --conntype serial --connstring /dev/ttyUSB0,baud=115200 reset

# After the new image boots successfully, confirm it permanently
mcumgr --conntype serial --connstring /dev/ttyUSB0,baud=115200 \
    image confirm
```

Or confirm from application code:

```c
boot_write_img_confirmed();   /* requires CONFIG_MCUBOOT_IMG_MANAGER=y */
```

> **Rollback**: If confirmation is not received before the next reset,
> MCUboot automatically reverts to the previous image in slot 0.

---

## Flash Partition Layout

| Flash | Partition | Label | Offset | Size |
|-------|-----------|-------|--------|------|
| Internal — nRF52840 (1 MiB) | `boot_partition` | `mcuboot` | `0x000000` | 48 KiB |
| Internal | `slot0_partition` | `image-0` | `0x00C000` | 976 KiB |
| External — MX25R1635F (2 MiB) | `slot1_partition` | `image-1` | `0x000000` | 976 KiB |
| External | `storage_partition` | `storage` | `0x0F4000` | 1 MiB |

MCUboot uses **swap-using-move**: slot 0 and slot 1 must be identical in size
(976 KiB). No scratch partition is required.

---

## Signing Key

The build uses `secrets/dfu_signing_dev.key` (ECDSA P-256) by default,
configured in `projects/platforms/zephyr.cmake`.

### Generate a new key

```bash
# ECDSA P-256 — matches mcuboot.conf default
imgtool keygen -k secrets/dfu_signing_prod.pem -t ecdsa-p256
```

> ⚠️ Never commit private keys. Ensure `secrets/*.pem` and `secrets/*.key`
> are in `.gitignore`.

### Switch to a different key

Update two places:

1. **`projects/platforms/zephyr.cmake`** — app signing key:

   ```cmake
   set(MCUBOOT_SIGNATURE_KEY_FILE
       "${CMAKE_CURRENT_LIST_DIR}/../../secrets/dfu_signing_prod.pem"
       CACHE STRING "MCUboot signing key" FORCE)
   ```

2. **MCUboot build** — pass the new key via `-DCONFIG_BOOT_SIGNATURE_KEY_FILE`
   and update `CONFIG_BOOT_SIGNATURE_TYPE_*` in `ports/zephyr/mcuboot.conf`
   if the algorithm changes:

   | Option | Algorithm |
   |--------|-----------|
   | `CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y` | ECDSA P-256 (default) |
   | `CONFIG_BOOT_SIGNATURE_TYPE_ED25519=y` | Ed25519 |
   | `CONFIG_BOOT_SIGNATURE_TYPE_RSA=y` | RSA-2048 |

Rebuild and reflash MCUboot after any key or algorithm change — the old
bootloader rejects images signed with a different key.

---

## Reference

### CMake (alternative to west)

#### Build MCUboot

```bash
cmake -B build/mcuboot \
      -S $ZEPHYR_BASE/../bootloader/mcuboot/boot/zephyr \
      -DBOARD=madi_nrf52840 \
      -DBOARD_ROOT=$(pwd)/ports/zephyr \
      "-DEXTRA_CONF_FILE=$(pwd)/ports/zephyr/mcuboot.conf" \
      "-DCONFIG_BOOT_SIGNATURE_KEY_FILE=$(pwd)/secrets/dfu_signing_dev.key" \
      -G Ninja
cmake --build build/mcuboot
```

#### Build App

```bash
cmake -B build/app -DTARGET_PLATFORM=madi_nrf52840 -G Ninja
cmake --build build/app
```

### Board notes

- **Board name**: `madi_nrf52840`
- **Console**: RTT (Segger J-Link) — UART disabled by default
- **LED**: P0.20 (active-low, alias `led0`)
- **QSPI flash**: CS=P0.07, SCK=P0.08, IO0=P1.09, IO1=P1.08, IO2=P0.12, IO3=P0.06


