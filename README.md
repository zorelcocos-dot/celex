# Celex

Windows x64 C++ application with a DirectX 11 / Dear ImGui overlay.

## Requirements

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 with **Desktop development with C++**
- Windows 10/11 SDK
- MSVC v143 toolset

## Build

Open `star.sln`, select `Release | x64`, then build the solution. From a Developer PowerShell prompt, the equivalent command is:

```powershell
msbuild star.sln /m /p:Configuration=Release /p:Platform=x64
```

The executable and its required fonts are copied to `build/`. Build products are intentionally ignored by Git.

## Runtime

- The overlay starts immediately and waits for Roblox when it is not running.
- Closing and reopening Roblox no longer requires restarting the application; worker threads stop, state is cleared, and a new session is attached automatically.
- Press the configured menu key (Insert by default) to toggle the menu.
- Close the overlay window or press Ctrl+C in the console for a clean shutdown.

## Configs

Profiles are stored in `configs/` next to the executable. Config schema v2 persists all user-facing settings, validates value types and ranges, migrates legacy profiles, and uses temporary/backup files when replacing an existing profile.

Runtime-only values such as active targets, toggle state, cached addresses, and live offsets are not persisted.

## Layout

- `star/Memory/` — process lifetime and memory access
- `star/features/` — feature workers and render-time behavior
- `star/overlay/` — DirectX 11 and ImGui renderer
- `star/rbx/` — SDK types, state snapshots, caches, offsets, and configs
- `star/assets/` — runtime fonts copied by the Release build

## Features

The in-game menu has five tabs: **Aimbot**, **Visuals**, **Movement**, **Misc**, and **Triggerbot**.

- **Aimbot** — camera or mouse aim with target/air bone selection, FOV circle,
  prediction, smoothness (linear / ease / custom bezier curve), shake, stutter,
  sticky aim, teamcheck and knockcheck.
- **Visuals** — 2D/3D/corner boxes, tracers, skeleton, names, distance, health,
  head circles, headless mode and per-element colors/thicknesses.
- **Movement** — fly, walk-speed boost, and infinite jump, each with its own
  keybind, hold/toggle mode and tunable speed/power.
- **Triggerbot** — radius/range based auto-fire, delay, team/knock checks, and
  per-bodypart "advanced FOV" hitboxes with live visualization.
- **Misc** — FOV override, crosshair, stream-proof capture exclusion, keybind
  list, config save/load, desync and hitbox expander.

## Tests

The platform-independent config round-trip, migration, clamping, filename validation, and atomic backup tests can be run with:

```bash
g++ -std=c++20 -Itests/stubs -I. tests/config_tests.cpp -o config-tests
./config-tests
```

The config tests are platform-independent. The complete application build remains an MSVC Release x64 check on Windows.

## Development notes

Do not commit generated executables, PDBs, object files, Visual Studio user settings, runtime configs, or cache files. The Windows CI build is the source-of-truth build check.
