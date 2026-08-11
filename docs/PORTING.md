# Native Windows port and verification

## Baseline

The engine starts from LAB3D/SDL 3.0.1. It is compiled as native C for Windows
with Visual Studio and uses the bundled sdl12-compat development/runtime set,
which supplies the SDL 1.2 interface over current SDL runtimes. Rendering is
available through OpenGL or the upstream high-resolution software path;
AdLib emulation, Windows MIDI, input, timers, and PCM sound are native APIs,
not DOS emulation.

## Windows x64 changes

- Added a root CMake build with x64/x86 imported SDL libraries, Windows system
  libraries, resource compilation, post-build data deployment, and install
  rules.
- Changed startup to use the executable's own directory before loading data,
  so shortcuts and launches from other working directories work reliably.
- Gave a fresh installation usable 1280x720 windowed, nearest-filter defaults;
  `-setup` remains available for every setting.
- Made Windows MIDI callback data use `DWORD_PTR` and removed a pointer-to-int
  increment that truncated addresses on 64-bit builds.
- Supplied the OpenGL `GL_CLAMP_TO_EDGE` constant missing from the legacy
  Windows OpenGL 1.1 header.
- Selected the static MSVC runtime to match the supplied SDL entry library and
  made all runtime DLL/data copying deterministic.
- Added a native in-game cheat menu on `Ctrl+Alt+F1`, including persistent god
  mode, bounded noclip, resource/powerup refills, full health, and next-board
  access without requiring the legacy cheat command-line switches.

The gameplay, board logic, original KZP data loading, saves, sound effects,
KSM sequencing, AdLib synthesis, and rendering algorithms remain in the
LAB3D/SDL source lineage.

## Verification performed

1. Configured a Visual Studio 2022 x64 CMake build and compiled Release.
2. Confirmed the PE executable launches from a working directory other than
   its data directory and remains alive with the correct window title.
3. Inspected the actual Windows window: board 1 loaded and the 3D view, textures,
   sprite transparency, perspective, palette, and HUD rendered correctly at
   1280x720-class window size.
4. Rebuilt the exact MZ analysis and NASM-round-tripped its 105,143-byte load
   module to the original SHA-256.
5. Validated asset counts, dimensions, map structure, MIDI/WAV headers, story
   sections, and representative decoded images through the automated tests.
6. Installed a self-contained distribution and smoke-launched the installed
   executable using only files in that directory.

## Known compatibility notes

- The settings, saves, and screenshots are intentionally stored beside the
  executable, matching the portable character of the upstream port. Put the
  distribution in a user-writable directory.
- The bundled engine is intentionally faithful rather than a rewrite. Some
  menus and input behavior retain early-1990s conventions.
- General MIDI uses a best-effort instrument map. Select AdLib emulation in
  setup for sound closest to the original OPL2 playback.
- The legal terms prohibit commercial distribution and require Internet-only,
  free-of-charge distribution of derivatives.
