# Linux desktop integration

1. Build the game (`./build_pc.sh` from the repository root).
2. Copy `AnimalCrossing.desktop` to `~/.local/share/applications/`.
3. Replace both `REPLACE_WITH_PC_BUILD32_BIN_DIR` placeholders with the **absolute** path to `pc/build32/bin` (the directory that contains `AnimalCrossing`, `rom/`, `shaders/`, …), for example `/home/you/cozyacres/pc/build32/bin`.
4. Run `update-desktop-database ~/.local/share/applications` if your desktop environment does not pick it up immediately.

The `.desktop` file is optional; you can always run `./AnimalCrossing` from a terminal inside `pc/build32/bin`.
