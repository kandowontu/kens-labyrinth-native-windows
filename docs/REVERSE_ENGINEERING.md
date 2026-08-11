# Reverse engineering report

## Provenance

The input was copied without modification from
`\\192.168.1.176\sdcard\games\AO486\media\Ken's Labyrinth\kens.vhd` and
preserved as `original/kens.vhd`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `kens.vhd` | 33,546,752 | `72FB5B5856169FF43F426BC362ACBEE0E6A49E8BED74FFBE24489B2D827B5F99` |
| installed `KEN.EXE` | 110,775 | `6D33F1502C80CDF2307B86AE77662F478F3C537367631043489442D373BB75C1` |
| MZ load module | 105,143 | `DD3D9A51E5C595E6B781685AF57F400F0CB4F927BB8B369C7B7118BBE37E35CB` |

The installed executable matches the official registered v2.1 `KEN.EXE`
byte-for-byte. It is not the separately distributed `KENSBFIX.EXE` sound-card
hotfix. Ken Silverman's released full-version source therefore provides direct
ground truth for the program's behavior and data structures.

## Executable structure

`KEN.EXE` is a 16-bit DOS MZ executable produced by Microsoft C 6.00A using
the medium memory model: code pointers are far and data pointers are near.

| Field | Value |
| --- | ---: |
| Header size | 5,632 bytes (`0x1600`) |
| Relocations | 1,349 |
| Minimum extra allocation | `0x0EE1` paragraphs |
| Initial `SS:SP` | `280C:0800` |
| Initial `CS:IP` | `171B:001A` |
| Entry module address | `0x171CA` |
| Entry file offset | `0x187CA` |

Module-linear addresses in the reports use `segment * 16 + offset`. At load
time DOS adds the process load segment to every segment word named in the MZ
relocation table.

## Disassembly deliverables

`tools/disassemble_mz.py` parses the header and all fixups, extracts the exact
load module, finds direct far calls/jumps whose segment operands have fixups,
and recursively follows segmented near/far control flow. It emits:

- `KEN.EXE.ndisasm.asm`, an exhaustive 16-bit linear decode of all 105,143
  load-module bytes. It necessarily decodes embedded data as instructions too.
- `KEN.EXE.byte-map.asm`, a lossless relocation-aware map. Classified
  instructions are annotated with `CS:IP`; other regions remain data. Every
  original byte is emitted once as `db`, so NASM can reconstruct the module.
- exact header, relocation, far-target, entry-point, control-flow, overlap, and
  coverage reports in JSON/CSV.
- an index of all 70 functions in the officially released original source.

The exact-byte map covers 100% of the load module and assembles back to the
same SHA-256. Recursive control-flow analysis classifies 96,158 bytes (91.45%)
as code, with 34,786 logical instructions, 1,319 fixed-up far targets, and
7,017 direct control-flow edges. The remaining 8,985 bytes are retained as
data or unclassified bytes rather than being mislabeled.

The original linker MAP and debug symbols were not released. Source function
names are therefore useful behavioral ground truth, but the reports do not
pretend that an unproven source name belongs to a particular binary address.

## Original engine organization

| Original file | Responsibility |
| --- | --- |
| `INIT.C` | DOS/VGA/sound initialization, command-line handling, memory setup |
| `GRAPHX.C` | Mode X renderer, ray casting, walls, masked planes, sprites, HUD |
| `LAB3D.C` | Main loop, object simulation, combat, collision, board progression |
| `SUBS.C` | Archive I/O, board/wall loading, sound/music, GIF, menus, story, saves |
| `LAB3D.H` | Global structures, wall/object identifiers, shared constants |

The renderer casts adaptive horizontal rays through a 64x64 grid, stores one
wall height per screen column as a compact depth buffer, then draws masked
walls and billboarding sprites farthest-first. The v2 monster pathfinder uses
a breadth-first search from the player's square. Music timing is based on a
240 Hz PIT interrupt and KSM event timestamps. The native port preserves these
game systems while replacing VGA, interrupts, port I/O, and DOS memory APIs.

## Native lineage

`src/lab3d` is based on LAB3D/SDL 3.0.1, the port recommended from Ken
Silverman's official Ken's Labyrinth page. It retains the original game logic
and data formats while providing SDL input/audio/timing, OpenGL or software
rendering, AdLib emulation, and Windows MIDI. Port-specific x64 changes are
documented in `PORTING.md`.

