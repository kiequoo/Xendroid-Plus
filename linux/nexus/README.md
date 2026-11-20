# Xenia Nexus

**An Xbox 360-styled dashboard launcher for arm64 handhelds, bundled with a native arm64 xenia build.**

Boot into an Xbox 360 Metro-style library on your AYN Odin 3 (or any arm64 Linux handheld running Armada OS), browse your games with a d-pad, and launch into the emulator — all from one AppImage.

A game-path argument still boots straight to the emulator for frontends (ES-DE, Pegasus, etc.).

---

## What it is

| | |
|---|---|
| **AppImage** | `XeniaNexus-arm64.AppImage` |
| **Routing** | No arg → Nexus dashboard; game path arg → `xenia_edge` directly |
| **Target** | AYN Odin 3 / Armada OS (arm64 Linux, Vulkan via Mesa Turnip) |
| **Emulated console** | Xbox 360 |

The launcher scans your Xbox 360 ROM folder (`.iso`, `.xex`, extracted `default.xex` folders), shows a game grid with controller navigation, and executes the bundled `xenia_edge` on select.  On exit the Nexus regains focus and returns to the grid.

---

## Code borrowed / attribution

This project is built on the shoulders of several excellent open-source projects.

### Emulator core
- **[xenia](https://github.com/xenia-project/xenia-edge)** — Xbox 360 emulator core, `src/`, `third_party/`.  
  Copyright © Ben Vanik and contributors. **BSD-3-Clause**.
- **[Xendroid-Plus](https://github.com/phforner0/Xendroid-Plus)** (this repository) — arm64/Snapdragon performance work and the Android integration layer wrapped around the xenia tree.  
  Copyright © phforner0 and contributors. **GNU AGPL-3.0**.

### Launcher UI
- **[xbased](https://github.com/crazyriddler/xbased)** — console-style SDL3 dashboard, via the `kiequoo/xbased-arm64` fork.  
  Copyright © crazyriddler and contributors. **GPL-3.0-only**.  
  xbased is itself based on **hakuX** by the hakuX contributors (GPL-3.0).

  The following files are vendored from xbased under `linux/nexus/ui/` and adapted for Xenia Nexus:

  | File | Modifications |
  |---|---|
  | `ui.c` / `ui.h` | Overlay path (swapchain.h) replaced by `nexus_swapchain_stub.h`; palette changed to Xbox 360 Metro (see `ui.h`); font paths changed to Selawik |
  | `dash.c` | `D.exe` resolution changed to `$APPDIR/usr/bin/xenia_edge`; branding changed to "Xenia Nexus"; xbased_version.h removed |
  | `library.c` | File extensions changed from Xbox-OG (`.xiso`, `.cci`, `.chd`, `default.xbe`) to Xbox 360 (`.xex`, `default.xex`); libretro thumbnail system changed to `Microsoft_-_Xbox_360` |
  | `config.c` | Data directory changed from `xbased` to `xenia-nexus` |
  | `dash_settings.c` | Rewritten as a Phase 1 stub (all screen functions pop the stack); kernel dependencies removed |
  | All other `ui/*.c` | Include paths changed from `native/app/ui/xxx.h` to `xxx.h` |

  The following xbased files are **not included**: `console_cfg.c`, `update.c`, `shortcut.c`, `xemu_import.c`, `cheat_text.c`, `dash_memory.c`, `win_mods.c`, `mod_fit.c`.  
  Stub headers replace `console_cfg.h`, `update.h`, and `shortcut.h`.

  `xbox_data.c` is replaced by `linux/nexus/xbox_data_shim.c` — a filename-only metadata implementation that requires no disc image parsing.

### Font
- **[Selawik](https://github.com/microsoft/Selawik)** — Microsoft's open-source, Segoe-metric-compatible font.  
  Copyright © Microsoft. **SIL Open Font License 1.1 (OFL)**.  
  Ships under `linux/nexus/ui/fonts/`.

### Support libraries
- **[SDL3](https://github.com/libsdl-org/SDL)** — reused from the xenia vendored tree (`third_party/SDL3`).  
  **zlib license**.
- **[stb](https://github.com/nothings/stb)** (`stb_truetype.h`, `stb_image.h`, `stb_image_write.h`) — vendored under `linux/nexus/third_party/stb/`.  
  **Public domain / MIT**.

---

## Licensing

The combined work is distributed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

- Vendored launcher UI files retain their original **GPL-3.0** headers.  
  GPLv3 and AGPLv3 are compatible via Section 13 of each license.
- xenia core files retain their **BSD-3-Clause** headers.
- Selawik fonts retain their **SIL OFL 1.1** license.

See [`LICENSE`](../../LICENSE) for the full AGPL-3.0 text.

---

## Building

Built automatically by `.github/workflows/linux-arm64.yml` on a native `ubuntu-22.04-arm` runner.

```bash
# 1. Build xenia_edge first (requires a full xenia configure + build):
#    (see CI workflow for the full command)

# 2. Build the Nexus launcher:
cmake -S linux/nexus -B linux/nexus/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang-21
cmake --build linux/nexus/build --config Release
```

The CI bundles both `xenia_edge` and `xenia-nexus` into one AppDir and emits `XeniaNexus-arm64.AppImage` via linuxdeploy.

---

## Installing (Odin 3 / Armada OS)

```bash
# Download the AppImage from the latest CI release and copy to the device:
scp XeniaNexus-arm64.AppImage armada@192.168.68.30:~/Applications/
ssh armada@192.168.68.30 "chmod +x ~/Applications/XeniaNexus-arm64.AppImage"
```

Launch without arguments for the dashboard, or with a game path for frontends:

```bash
# Dashboard mode
~/Applications/XeniaNexus-arm64.AppImage

# Frontend / direct mode  
~/Applications/XeniaNexus-arm64.AppImage /path/to/game.iso
```

---

## Credits

- [crazyriddler](https://github.com/crazyriddler) — xbased
- hakuX contributors — hakuX (the xbased ancestor)
- [phforner0](https://github.com/phforner0) — Xendroid-Plus arm64 performance work
- [Ben Vanik](https://github.com/benvanik) and the xenia team — xenia emulator core
- Microsoft — Selawik font
