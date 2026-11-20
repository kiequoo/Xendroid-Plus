# Xendroid-Plus — Linux arm64

Native Linux arm64 build of the xenia desktop emulator, carrying all of
Xendroid-Plus's core improvements (ARM64 JIT, Turnip/Adreno GPU work, Kernel/VFS
fixes, XMA audio, etc.).

Tested on: **AYN Odin 3 (Armada OS)** — Adreno 830, Mesa Turnip 26.2.6, Vulkan 1.4.

## Quick start (Odin 3 / Armada OS)

The Odin 3 uses an immutable Fedora bootc root (no host compiler). Build inside a
distrobox container so the base image stays untouched:

```bash
# 1. Create a build container (once)
distrobox create --name xenia-build --image ubuntu:22.04
distrobox enter xenia-build

# 2. Inside the container — install toolchain + deps (once)
UBUNTU_BASE=$(lsb_release -cs)
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
  | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
sudo add-apt-repository --no-update \
  "deb http://apt.llvm.org/${UBUNTU_BASE}/ llvm-toolchain-${UBUNTU_BASE}-21 main"
sudo add-apt-repository -y --no-update ppa:ubuntu-toolchain-r/test
sudo apt-get -y update
sudo apt-get -y install --no-install-recommends \
  clang-21 lld-21 libclang-rt-21-dev llvm-21
sudo apt-get -y install \
  g++-14 cmake ninja-build pkg-config \
  libgtk-3-dev libx11-dev libx11-xcb-dev libxext-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev libxss-dev libxkbcommon-dev \
  libxfixes-dev libxtst-dev libfontconfig1-dev \
  libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libcurl4-openssl-dev \
  python3 python3-pip git wget

# 3. Clone and build (the distrobox shares your home directory)
cd ~/dev/Xendroid-Plus
./linux/build.sh
```

The binary lands at:
`emulator-core/src/main/cpp/xenia/build/bin/Linux/Release/xenia-app`

Run a title:
```bash
./emulator-core/src/main/cpp/xenia/build/bin/Linux/Release/xenia-app \
    /run/media/armada/6532-3664/Emulation/roms/xbox360/roms/YourGame.iso
```

## Pulling upstream changes

```bash
git fetch upstream
git merge upstream/main
```

Conflicts will be confined to the files in `linux/` and
`.github/workflows/linux-arm64.yml` — upstream never touches those paths.
After merging, run `linux/build.sh` again to rebuild.

## Build options

| Env var | Default | Purpose |
|---|---|---|
| `CC` / `CXX` | `clang-21` / `clang++-21` | Compiler override |
| `XENDROID_NINJA_JOBS` | `nproc` | Parallel compile jobs |
| `XENDROID_THINLTO_JOBS` | `4` | ThinLTO backend threads (caps RAM use during link) |

## Notes

- The Android JNI layer (`emulator-core/src/main/cpp/xendroid*.cpp`, AAudio, ADPF,
  ZeroFG presenter) is not built. The desktop `xenia-app` target is fully self-contained
  inside the embedded xenia tree and has its own Linux entry point.
- SGSR1 (Snapdragon-specific upscaler) is Android-only and not available here.
- Audio uses SDL3 (PulseAudio/PipeWire backend on Armada OS). ALSA fallback works.
- Input uses SDL3 gamepad via evdev. The Odin 3's built-in controller is recognised
  as a standard gamepad.
