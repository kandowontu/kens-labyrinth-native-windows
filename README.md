# Ken's Labyrinth native Windows port

This workspace preserves and analyzes the installed registered **Ken's
Labyrinth v2.1** DOS release, expands its assets into common formats, and builds
a native 64-bit Windows executable. The port is based on the author-recommended
LAB3D/SDL lineage and is packaged with a modern SDL compatibility runtime; no
DOS emulator is required.

Original game, source code, and historical downloads:
[Ken Silverman's official Ken's Labyrinth page](https://advsys.net/ken/klab.htm).

## Run the finished build

Open `dist/KensLabyrinth/KensLabyrinth.exe`. It uses the data files beside the
executable and writes `settings.ini`, saved games, and screenshots there.

Run `KensLabyrinth.exe -setup` to change video, input, sound, music, and cheat
settings. Useful command-line options include `-win`, `-res 1280 720`,
`-nomusic`, and `-nosound`; the complete upstream option list is in
`src/lab3d/run.txt`.

Press **Alt+Enter** at any time in the game to toggle fullscreen mode.

Choose **Controls** on the main menu to remap all movement, action, weapon,
menu, cheat, screenshot, volume, and gamma keys. Bindings save immediately;
see [the controls guide](docs/CONTROLS.md).

While playing, press **Ctrl+Alt+F1** to open the native cheat menu. It provides
toggle-all, god mode, noclip, automatic money/key/life refills, maximum weapons,
permanent protection powerups, compass, full health, and next-board controls.
Activating a cheat marks the current run as cheated, preserving the original
high-score rules. See [the cheat guide](docs/CHEATS.md).

## Build on Windows

Requirements are CMake 3.24+ and Visual Studio 2022 with the Desktop C++
workload. SDL headers, import libraries, and runtime DLLs are included.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
cmake --install build --config Release --prefix dist/KensLabyrinth
```

The build starts in a 1280x720 window with nearest-neighbour filtering on a
fresh install. Use `-setup` to choose other defaults.

## Repository map

| Path | Contents |
| --- | --- |
| `disassembly` | Exact MZ header, fixups, full flat listing, relocation-aware byte map, and cross-references |
| `references/original-source` | Ken Silverman's officially released v2.1 C source |
| `assets/ripped/v2.1` | Lossless decompressed DAT, GIF, KSM, and WAV assets |
| `assets/converted/v2.1` | PNG graphics/maps, JSON catalogs, MIDI previews, WAV, story text, and hashes |
| `src/lab3d` | Native LAB3D/SDL engine source with Windows x64 fixes |
| `game-data/v2.1` | Original compressed runtime data used by the port |
| `tools` | Reproducible asset-rip and MZ-disassembly scripts |

The exact installed `KEN.EXE` is byte-for-byte identical to the official v2.1
full-release executable, SHA-256
`6D33F1502C80CDF2307B86AE77662F478F3C537367631043489442D373BB75C1`.
The source VHD was used for local preservation and inventory, but is excluded
from the public GitHub repository; its recorded SHA-256 is
`72FB5B5856169FF43F426BC362ACBEE0E6A49E8BED74FFBE24489B2D827B5F99`.

## Reproduce the analysis

Install the analysis-only Python packages:

```powershell
python -m pip install -r requirements-analysis.txt
python tools/disassemble_mz.py references/official-full/KEN.EXE disassembly
```

The disassembler also expects `ndisasm` on `PATH`; `nasm` is used to verify
that the exact-byte map assembles back to the original load module.

The lossless archives were expanded with KKIT/CLI 2.1 support. To rebuild the
common-format view from those expanded files:

```powershell
python tools/rip_assets.py assets/ripped/v2.1 assets/converted/v2.1 `
  --source-header references/original-source/LAB3D.H `
  --gm-map src/lab3d/ksmmidi.txt
```

Run the artifact checks with `python -m unittest discover -s tests -v`.

More detail is in [reverse engineering](docs/REVERSE_ENGINEERING.md), [asset
formats](docs/ASSET_FORMATS.md), [cheats](docs/CHEATS.md), and
[porting/testing](docs/PORTING.md).

The complete contributor and dependency acknowledgements are in
[CREDITS.md](CREDITS.md).

## License and distribution

This is a non-commercial derivative. Ken Silverman's source license permits
modified distributions **only through the Internet, free of charge, with no
commercial exploitation**. Any distribution using the source must preserve
the required copyright and official-site notice. The complete terms are in
`NOTICE.md` and `src/lab3d/readme.txt`; the SDL license is retained in
`third_party/sdl12-compat` and installed as `LICENSE-SDL.txt`.

"Ken's Labyrinth" Copyright (c) 1992-1993 Ken Silverman  
Ken Silverman's official web site: <http://www.advsys.net/ken>
