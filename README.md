# Animal Crossing PC Port

A native PC port of Animal Crossing (GameCube) built on top of the [ac-decomp](https://github.com/ACreTeam/ac-decomp) decompilation project.

The game's original C code runs natively on x86, with a custom translation layer replacing the GameCube's GX graphics API with OpenGL 3.3.

This repository does not contain any game assets or assembly whatsoever. An existing copy of the game is required.

Supported versions: GAFE01_00: Rev 0 (USA)

## Quick Start (Pre-built Release)

Pre-built releases are available on the [Releases](https://github.com/flyngmt/ACGC-PC-Port/releases) page. No build tools required.

1. Download and extract the latest release zip
2. Place your disc image in the `rom/` folder
3. Run `AnimalCrossing.exe`

The game reads all assets directly from the disc image at startup. No extraction or preprocessing step is needed.

## Building from Source

Only needed if you want to modify the code. Otherwise, use the [pre-built release](https://github.com/flyngmt/ACGC-PC-Port/releases) above.

The PC port is **32-bit only** (the decomp relies on pointer-to-`u32` patterns). On 64-bit Linux you need multilib (gcc `-m32`) or an i686 cross toolchain, plus **32-bit** SDL2 and OpenGL development libraries.

### Requirements

- **Animal Crossing (USA) disc image** (ISO, GCM, or CISO format)
- **Windows:** [MSYS2](https://www.msys2.org/) with the **MINGW32** (32-bit) environment
- **Linux:** a 32-bit-capable GCC/Clang toolchain, 32-bit SDL2, and 32-bit OpenGL/Mesa dev packages (see below)

### Windows (MSYS2)

Open **MSYS2 MINGW32** from your Start menu and install:

```bash
pacman -S mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-SDL2 mingw-w64-i686-make
```

Build:

1. Clone the repository and `cd` into it.

2. From the **MINGW32** shell:
   ```bash
   ./build_pc.sh
   ```

3. Place your disc image under `pc/build32/bin/rom/` (for example `YourGame.ciso`).

4. Run:
   ```bash
   pc/build32/bin/AnimalCrossing.exe
   ```

### Linux (native)

The shipped executable is always **32-bit x86 (i686)**. On **x86_64 Ubuntu** it runs fine; you need **multilib / i386** build libraries and, to run it, matching **i386 runtime** libraries (SDL2, OpenGL).

#### Ubuntu on x86_64 (do this first)

1. Clone the repo and `cd` into it.

2. Install build dependencies (adds `i386` if needed, prefers `gcc -m32`, otherwise installs `i686-linux-gnu-*`). Add `--runtime` to also install the 32-bit SDL/OpenGL libraries required **to run** the game:

   ```bash
   ./scripts/install-linux-pc-deps.sh --runtime
   ```

   Or manually:

   ```bash
   sudo dpkg --add-architecture i386   # once, if not already
   sudo apt update
   sudo apt install build-essential cmake ninja-build pkg-config \
     gcc-multilib g++-multilib \
     libsdl2-dev:i386 libgl1-mesa-dev:i386
   ```

3. Build (or run **`./compile.sh`** — it picks native build on x86_64 and Docker on ARM):

   ```bash
   ./build_pc.sh
   ```

4. If you used `./scripts/install-linux-pc-deps.sh` **without** `--runtime`, install 32-bit runtime libs (**x86_64 Ubuntu only**):

   ```bash
   ./scripts/install-linux-runtime-i386.sh
   ```

   (or: `sudo apt install libsdl2-2.0-0:i386 libgl1:i386`)

5. Optional: install a launcher — see `packaging/linux/README.md`.

6. Put your USA disc image under `pc/build32/bin/rom/` (`.iso`, `.gcm`, or `.ciso`).

7. Run:

   ```bash
   pc/build32/bin/AnimalCrossing --verbose
   ```

If CMake cannot find SDL2, ensure `PKG_CONFIG_LIBDIR` points at the directory that contains `sdl2.pc` for i386 (often `/usr/lib/i386-linux-gnu/pkgconfig`); see `pc/cmake/Toolchain-linux32.cmake`.

#### Raspberry Pi and other ARM64

There is **no ARM-native** target in this repository yet (the game code assumes **x86** and 32-bit pointers). A Raspberry Pi **cannot** run this binary natively.

- **Playing on a Pi:** use remote desktop / game streaming from an **x86_64** machine that runs the game, or third-party x86 emulation (e.g. Box86, qemu-user) — unsupported here and often too slow for this title.
- **Building on a Pi:** host `apt` cannot install `libsdl2-dev:i386` like on amd64. Use a **linux/amd64** container (slower on ARM):

  ```bash
  ./scripts/docker-build-linux-amd64.sh
  ```

  The result is still an **x86** binary; copy it to an x86_64 Ubuntu/PC to run.

#### "Unable to locate package libsdl2-2.0-0:i386"

Run **`./scripts/diagnose-apt-i386.sh`**.

- If apt prints **`Ignoring file ... invalid filename extension`** for **`ubuntu.sources.bak-before-i386-*`** (or other **`*.sources.bak*`**), run **`sudo ./scripts/apt-cleanup-sources-list-d.sh`** (default: only those patterns). Use **`sudo ./scripts/apt-cleanup-sources-list-d.sh --all-bak`** only if you also want **`*.list.bak`**, **`*~`**, **`*.old`**, etc. moved to **`/var/backups/apt-sources/`** (recover from there if needed).
- If **`dpkg --print-architecture`** is **`arm64`**, you are on **ARM64** — run the game on **x86_64** Linux.
- On **amd64**, Ubuntu **22.04+** often stores mirrors in **`*.sources`** (DEB822) with **`Architectures: amd64`** or **`amd64 arm64`** and **no `i386`**, so apt **never downloads** the i386 package index. `dpkg --add-architecture i386` alone is not enough.  
  **Fix:** `sudo ./scripts/fix-ubuntu-apt-i386.sh` then `sudo apt install libsdl2-2.0-0:i386 libgl1:i386`
- Legacy **`deb`** lines instead need **`[arch=amd64,i386]`** on `archive.ubuntu.com` entries.

#### Build fails: **`SDL_HINT_VIDEO_DRIVER` undeclared**

SDL2 defines **`SDL_HINT_VIDEODRIVER`** (no underscore between **`VIDEO`** and **`DRIVER`**). If you see this error, **`pc/src/pc_main.c`** is out of date — sync with upstream or replace the wrong token with **`SDL_HINT_VIDEODRIVER`**.

#### Segmentation fault immediately on launch

- From **`pc/build32/bin`**, run **`./AnimalCrossing --verbose`** and note the last line printed before the crash.
- **Linux i686 (current `pc_main.c`):** only **`SDL_INIT_VIDEO`** runs until after **`gladLoadGL`**; then audio/timer/gamecontroller subsystems start. **Optional** LLVM preload: set **`PC_LLVM_PRELOAD=1`** to **`dlopen` `libLLVM`** with **`RTLD_GLOBAL`** before **`SDL_Init`** (can help on some Mesa setups; on others the **first** **`dlopen` of `libLLVM` SIGSEGVs** in LLVM static init — then leave it unset). **Rebuild** with **`./build_pc.sh`**. To try EGL on X11 instead of GLX: **`PC_X11_EGL=1`** or **`./scripts/run-pc-linux-x11-egl.sh`** (still loads LLVM via Mesa on many drivers). **Container workaround:** **`./scripts/docker-run-pc-ubuntu2404.sh`** builds and runs with **24.04**’s i386 GL stack.
- If **`gdb`** still shows **`libLLVM`** and **`llvm::ManagedStaticBase::RegisterManagedStatic`** while **`glXChooseVisual`** (GLX) or **`eglGetProcAddress`** (EGL) is on the stack, the fault is still in **Mesa’s i386 stack** (not game logic). A typical stack ends with **`dlopen` → `libGLX.so` → `glXChooseVisual` → `libSDL2` → `pc_platform_init` → `main`**. Some Ubuntu versions (e.g. **25.10**) may not publish **`mesa-utils:i386`** / **`glxgears:i386`**; rely on **`gdb`** on **`AnimalCrossing`** instead. For a distro bug report, capture: **`dpkg-query -W libllvm20 libgl1-mesa-dri libglx-mesa0 mesa-libgallium`** (exact names vary) and **`apt policy libllvm20:i386 libgl1-mesa-dri:i386`**. Further mitigations: **different Mesa/LLVM combo** (another Ubuntu release, container, or older **`libllvm*t:i386`** / **`libgl1-mesa-dri:i386`**), or a machine where **i386 GL** is known good.
- If **`gdb`** shows the fault in **`/opt/amdgpu/.../libLLVM.so.*amdgpu`** while **`glXChooseVisual`** / **`libGLX`** is **`dlopen`**-ing the driver, the **AMDGPU i386** stack is being loaded (often broken vs Ubuntu 25.10 + Mesa). **`LIBGL_ALWAYS_SOFTWARE=1` alone does not fix that** because GLX still pulls AMDGPU first.
  - The game used to export **`DRI_PRIME=1`** on every Linux launch; that can trigger the bad GLX path on **32-bit**. Current sources skip that on i686 (use **`PC_USE_DISCRETE_GPU=1`** if you need prime on a laptop). **Rebuild** with **`./build_pc.sh`** so that fix is in your binary.
  - **Wayland + 32-bit SDL** often crashes on Ubuntu; force X11: **`SDL_VIDEODRIVER=x11`**. Try **`./scripts/run-pc-linux.sh`** (GPU Mesa) or **`./scripts/run-pc-linux-safegl.sh`** (llvmpipe). Both set X11 + **`DRI_PRIME=0`** + Mesa-friendly **`LD_LIBRARY_PATH`**.
  - **Multisample (MSAA)** is **off by default on Linux** (even if **`settings.ini`** says **`msaa=4`**); set **`PC_MSAA=1`** to opt in. If it still dies in **`pc_platform_init`**, try **`./scripts/run-pc-linux.sh`** or the software GL one-liner below.
  - **`ninja: mkdir(/src): Permission denied`** means **`pc/build32`** was configured inside Docker (`/src/pc`). **`./build_pc.sh`** removes that tree when the cache path mismatches; if you see **`rm: ... Permission denied`**, the tree is **root-owned**: run **`sudo rm -rf pc/build32`**, then **`./build_pc.sh`**. New **`docker-build-linux-amd64.sh`** runs **`chown`** on **`pc/build32`** after the build so this should not recur.
  - After removing **`amdgpu-lib32`**, run **`sudo apt autoremove`** so orphaned **`libllvm*-amdgpu:i386`**, **`mesa-amdgpu-libgallium:i386`**, etc. are not still pulled into the process.
  - One-liner (software): **`SDL_VIDEODRIVER=x11 DRI_PRIME=0 __GLX_VENDOR_LIBRARY_NAME=mesa LIBGL_DRIVERS_PATH=/usr/lib/i386-linux-gnu/dri LD_LIBRARY_PATH=/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu LIBGL_ALWAYS_SOFTWARE=1 ./AnimalCrossing`**
  - Last resort for 32-bit GL only: remove AMDGPU’s i386 GL packages, e.g. **`sudo apt remove libgl1-amdgpu-mesa-dri:i386`** (and related **`mesa-amdgpu-*:i386`** if you do not need them); keep the normal **`libgl1-mesa-dri:i386`** stack.
- Rebuild on the same OS so libc/SDL match: **`./build_pc.sh`** after install scripts.
- For a backtrace: from the repo root run **`./scripts/gdb-pc-linux.sh --verbose`**, then at the **`(gdb)`** prompt type **`run`**, and after the fault **`bt`**.  
  You must pass the **real path** to the binary (e.g. **`~/cozyacres/pc/build32/bin/AnimalCrossing`**): **`gdb /pc/build32/...`** is wrong because **`/pc`** is not your home directory.  
  Do not type **`gdb /path`** *inside* **`(gdb)`** (that is not a shell). Use **`file /path/to/AnimalCrossing`** to switch executables, or **`quit`** and run the helper again.
- To attach **debug symbols** to the game binary (not Mesa): rebuild with **`COZY_PC_GDB_SYMBOLS=1 ./build_pc.sh`** (adds **`-g`**, keeps default **`-O0`** from this project’s CMake setup).
- For a **Launchpad / distro bug** about the **`libLLVM` + `glXChooseVisual`** crash, run **`./scripts/collect-pc-linux-gl-bug-info.sh`** and paste the output.
- **Workaround host:** **`./scripts/docker-run-pc-ubuntu2404.sh`**, or a **24.04** VM / **`distrobox`**, with **`i386`** GL packages; **questing** (25.10) has been observed to fault in **`libLLVM.so.20.1`** during the first Mesa load (**GLX** or **EGL**).

If you ran **`fix-ubuntu-apt-i386.sh`** before backups were moved to **`/var/backups/apt-sources`**, remove any stray **`ubuntu.sources.bak-before-i386-*`** file still under **`/etc/apt/sources.list.d/`**. Apt will **ignore** files there with extensions it does not recognize and may print **`Ignoring file ... invalid filename extension`** — move those backups to **`/var/backups/apt-sources/`** (or delete them if you no longer need them).

## Controls

Keyboard bindings are customizable via `keybindings.ini` (next to the executable). Mouse buttons (Mouse1/Mouse2/Mouse3) can also be assigned.

### Keyboard (defaults)

| Key | Action |
|-----|--------|
| WASD | Move (left stick) |
| Arrow Keys | Camera (C-stick) |
| Space | A button |
| Left Shift | B button |
| Enter | Start |
| X | X button |
| Y | Y button |
| Q / E | L / R triggers |
| Z | Z trigger |
| I / J / K / L | D-pad (up/left/down/right) |

### Gamepad

SDL2 game controllers are supported with automatic hotplug detection. Button mapping follows the standard GameCube layout.

## Command Line Options

| Flag | Description |
|------|-------------|
| `--verbose` | Enable diagnostic logging |
| `--no-framelimit` | Disable frame limiter (unlocked FPS) |
| `--model-viewer [index]` | Launch debug model viewer (structures, NPCs, fish) |
| `--time HOUR` | Override in-game hour (0-23) |

## Settings

Graphics settings are stored in `settings.ini` and can be edited manually or through the in-game options menu:

- Resolution (up to 4K)
- Fullscreen toggle
- VSync
- MSAA (anti-aliasing)
- Texture Loading/Caching (No need to enable if you aren't using a texture pack)

## Texture Packs

Custom textures can be placed in `texture_pack/`. Dolphin-compatible format (XXHash64, DDS).

I highly recommend the following texture pack from the talented artists of Animal Crossing community.

[HD Texture Pack](https://forums.dolphin-emu.org/Thread-animal-crossing-hd-texture-pack-version-23-feb-22nd-2026)

## Save Data

Save files are stored in `save/` using the standard GCI format, compatible with Dolphin emulator saves. Place a Dolphin GCI export in the save directory to import an existing save.

## Credits

This project would not be possible without the work of the [ACreTeam](https://github.com/ACreTeam) decompilation team. Their complete C decompilation of Animal Crossing is the foundation this port is built on.

## AI Notice

AI tools such as Claude were used in this project (PC port code only).
