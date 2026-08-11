# Asset extraction and formats

All registered v2.1 runtime assets are preserved in their original compressed
form under `game-data/v2.1`. KKIT/CLI expands the KZP containers losslessly into
`assets/ripped/v2.1`; `tools/rip_assets.py` then produces the common-format
catalog under `assets/converted/v2.1`.

## Inventory

| Kind | Lossless form | Converted form | Count |
| --- | --- | --- | ---: |
| Textures, sprites, font/UI tiles | `WALLS.DAT` | indexed transparent PNG + JSON + sheet | 448 |
| Boards | `BOARDS.DAT` | 512x512 PNG overview + JSON tile grid | 30 |
| Music | KSM | original KSM + Standard MIDI preview | 39 |
| OPL2 instruments | `INSTS.DAT` | JSON register records | 256 |
| Digitized sounds | WAV | original WAV + JSON metadata | 33 |
| Full-screen art | GIF | original GIF + RGBA PNG | 3 |
| Story/menu text | `STORY.DAT` | UTF-8 text + JSON sections | 65 sections |
| Lookup tables | `TABLES.DAT` | original binary | 1 |

`assets/converted/v2.1/manifest.json` contains sizes and SHA-256 values for the
source files plus the authoritative counts above. The original extracted files
are never overwritten.

## KZP containers

`WALLS.KZP`, `BOARDS.KZP`, `LAB3D.KZP`, `SONGS.KZP`, `SOUNDS.KZP`, and
`STORY.KZP` use the game's LZW-family compression and archive framing. The
retained KKIT/SDL source under `references/klabkit-sdl` is the reproducible
decompressor used here. `LAB3D.KZP` yields the three modified-GIF screen images.

## Walls and sprites

`WALLS.DAT` is 1,836,032 bytes:

- bytes `0..447`: one flags byte for each image;
- bytes `448..1023`: reserved/padding;
- bytes `1024..`: 448 images of 64x64 8-bit palette indices.

Pixels are column-major (`data[x*64+y]`). The converter transposes them into
normal PNG scanline order, applies the v2 in-game palette from `LAB3D-002.gif`,
and marks palette index 255 transparent. Flag bits 0-2 describe cube, plane, or
directional behavior; bit 3 marks inside/no-clip behavior and bit 4 marks a
destructible image. `LAB3D.H` supplies names for known game IDs. `walls.json`
retains the raw flag and per-image hash.

The images are a unified atlas: world textures, doors and masked planes,
monsters, projectiles, pickups, character glyphs, menus, and HUD fragments all
use the same 64x64 record format. `wall_sheet.png` is a quick visual index.

## Boards

`BOARDS.DAT` holds 30 consecutive 64x64 maps. Each tile is a little-endian
16-bit word in the original `board[x][y]` order, for 8,192 bytes per map.

- bits 0-9: one-based wall/object image ID;
- bit 10: inside/no-clip behavior;
- bit 11: destructible;
- bit 12: player start marker (low two bits then encode facing);
- bit 13: vertical orientation for applicable planes;
- bits 14-15: retained verbatim as `bit_14` and `bit_15`.

Converted JSON uses conventional `rows[y][x]`, but includes the source layout,
raw 16-bit values, decoded flags, player start, episode, and level number. PNG
overviews use the actual texture art and show the start direction in red.

## KSM music and instruments

Each KSM begins with five 16-byte track tables: instrument, quantization,
channel count, priority, and volume. A little-endian 16-bit note-event count
follows at byte 80. Each four-byte event stores the six-bit note, on/off bit,
track number, and 20-bit tick value. Ticks run at 240 Hz.

MIDI files are compatibility previews, not replacements for archival KSM.
Programs are mapped through `src/lab3d/ksmmidi.txt`; percussion tracks map to
General MIDI percussion. Archive padding after the declared event count is
reported rather than interpreted.

`INSTS.DAT` contains 256 records of 33 bytes: a 20-byte CP437 name, 11 OPL2
register bytes, and two retained bytes. JSON includes the name, register data,
and General MIDI mapping.

## Sound, screens, story, and tables

KKIT/CLI emits 33 standard PCM WAV files with their original samples. The
catalog records channel count, sample width, rate, frames, duration, and hash.

The three screen images are preserved as GIF and converted to RGBA PNG. The
palette from `LAB3D-002.gif` is also exported as `palette.png` and
`palette.json`.

Decompressed `STORY.DAT` is CP437 text separated into 65 backslash-delimited
sections; both normalized UTF-8 text and structured JSON are emitted.
`TABLES.DAT` is retained unchanged because it is a numeric runtime lookup
table, not a media asset.

