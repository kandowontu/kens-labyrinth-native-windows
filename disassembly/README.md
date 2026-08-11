# Exact DOS executable disassembly

This directory analyzes `KEN.EXE` byte-for-byte. Its SHA-256 is
`6D33F1502C80CDF2307B86AE77662F478F3C537367631043489442D373BB75C1`. The executable is a 16-bit DOS MZ built with
Microsoft C 6.00A's medium memory model (far code, near data).

## Artifacts

- `KEN.EXE.ndisasm.asm`: exhaustive flat 16-bit decoding of every byte in the
  MZ load module. It is intentionally over-inclusive: data bytes also decode as
  instructions in any flat disassembly.
- `KEN.EXE.byte-map.asm`: relocation-aware recursive traversal. Exact `db`
  bytes make it lossless; decoded instructions and segmented addresses appear
  beside them. Unreached bytes are marked as data/unclassified.
- `ken_load_module.bin`: exact post-header load module, before DOS applies the
  relocation delta.
- `mz_header.json`, `relocations.csv`, `fixed_far_targets.csv`,
  `control_flow_edges.csv`, and `coverage.json`: machine-readable analysis.
- `source-functions.txt`: function index generated from the officially released
  original C source. The original linker MAP/debug symbols were not distributed,
  so source function names must not be presented as proven binary addresses.

## Address convention

Module-linear address is `segment * 16 + offset`; file offset is module-linear
plus the 0x1600-byte MZ header. DOS adds the process load
segment to each word listed in `relocations.csv`.

The entry point is `171B:001A` (module
`0x171CA`, file `0x187CA`).

## Reproduce

From the repository root:

```powershell
python tools/disassemble_mz.py original/Kens/KEN.EXE disassembly
```
