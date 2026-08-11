#!/usr/bin/env python3
"""Create reproducible, byte-covering analysis of the Ken's Labyrinth DOS MZ.

The executable is a segmented 16-bit program, so no single flat listing can
perfectly distinguish code from embedded tables.  This tool emits both:

* an exhaustive linear ndisasm listing (every load-module byte decoded), and
* a relocation-aware recursive traversal that follows near and far control
  flow from the entry point and every fixed-up direct far-call target.

It also preserves the untouched load module, MZ metadata, relocations, control
flow targets, and a byte classification report.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import shutil
import struct
import subprocess
import tempfile
from collections import deque
from dataclasses import dataclass
from pathlib import Path

from capstone import (
    CS_ARCH_X86,
    CS_GRP_CALL,
    CS_GRP_JUMP,
    CS_GRP_RET,
    CS_MODE_16,
    Cs,
)
from capstone.x86_const import X86_OP_IMM


HEADER_FIELDS = (
    "e_magic",
    "e_cblp",
    "e_cp",
    "e_crlc",
    "e_cparhdr",
    "e_minalloc",
    "e_maxalloc",
    "e_ss",
    "e_sp",
    "e_csum",
    "e_ip",
    "e_cs",
    "e_lfarlc",
    "e_ovno",
)


@dataclass(frozen=True)
class Relocation:
    index: int
    offset: int
    segment: int

    @property
    def linear(self) -> int:
        return self.segment * 16 + self.offset


@dataclass
class DecodedInstruction:
    cs: int
    ip: int
    linear: int
    size: int
    raw: bytes
    mnemonic: str
    operands: str


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_mz(exe: bytes) -> tuple[dict[str, int], bytes, list[Relocation]]:
    if len(exe) < 28:
        raise ValueError("file is too short to contain an MZ header")
    values = struct.unpack_from("<14H", exe)
    header = dict(zip(HEADER_FIELDS, values, strict=True))
    if header["e_magic"] != 0x5A4D:
        raise ValueError("input is not a DOS MZ executable")

    declared_size = (header["e_cp"] - 1) * 512 + (header["e_cblp"] or 512)
    header_size = header["e_cparhdr"] * 16
    if declared_size > len(exe):
        raise ValueError("MZ header declares a file larger than the input")
    if header_size > declared_size:
        raise ValueError("MZ header is larger than the declared executable")

    relocations: list[Relocation] = []
    relocation_end = header["e_lfarlc"] + header["e_crlc"] * 4
    if relocation_end > header_size:
        raise ValueError("relocation table extends beyond the MZ header")
    for index in range(header["e_crlc"]):
        offset, segment = struct.unpack_from("<HH", exe, header["e_lfarlc"] + index * 4)
        relocations.append(Relocation(index, offset, segment))

    header.update(
        {
            "declared_file_size": declared_size,
            "actual_file_size": len(exe),
            "header_size": header_size,
            "load_module_size": declared_size - header_size,
            "entry_linear": header["e_cs"] * 16 + header["e_ip"],
            "entry_file_offset": header_size + header["e_cs"] * 16 + header["e_ip"],
        }
    )
    return header, exe[header_size:declared_size], relocations


def fixed_far_targets(module: bytes, relocations: list[Relocation]) -> list[dict[str, int | str]]:
    """Find direct far CALL/JMP operands whose segment word has an MZ fixup."""
    targets: list[dict[str, int | str]] = []
    for relocation in relocations:
        site = relocation.linear
        if site < 3 or site + 2 > len(module):
            continue
        opcode = module[site - 3]
        if opcode not in (0x9A, 0xEA):
            continue
        offset = struct.unpack_from("<H", module, site - 2)[0]
        segment = struct.unpack_from("<H", module, site)[0]
        target_linear = segment * 16 + offset
        if target_linear >= len(module):
            continue
        targets.append(
            {
                "kind": "call" if opcode == 0x9A else "jump",
                "source_linear": site - 3,
                "source_file_offset": site - 3,
                "target_segment": segment,
                "target_offset": offset,
                "target_linear": target_linear,
                "relocation_index": relocation.index,
            }
        )
    return targets


def traverse(
    module: bytes,
    entry: tuple[int, int],
    far_targets: list[dict[str, int | str]],
) -> tuple[dict[tuple[int, int], DecodedInstruction], dict[tuple[int, int], set[str]], list[dict[str, int | str]]]:
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True

    queue: deque[tuple[int, int, str]] = deque()
    queue.append((entry[0], entry[1], "MZ entry point"))
    for target in far_targets:
        queue.append(
            (
                int(target["target_segment"]),
                int(target["target_offset"]),
                f"fixed-up far {target['kind']} at module+0x{int(target['source_linear']):05X}",
            )
        )

    decoded: dict[tuple[int, int], DecodedInstruction] = {}
    labels: dict[tuple[int, int], set[str]] = {}
    edges: list[dict[str, int | str]] = []
    queued: set[tuple[int, int]] = set()

    def enqueue(cs: int, ip: int, reason: str) -> None:
        key = (cs & 0xFFFF, ip & 0xFFFF)
        labels.setdefault(key, set()).add(reason)
        if key not in decoded and key not in queued:
            queued.add(key)
            queue.append((key[0], key[1], reason))

    while queue:
        cs, start_ip, reason = queue.popleft()
        queued.discard((cs, start_ip))
        labels.setdefault((cs, start_ip), set()).add(reason)
        ip = start_ip

        while True:
            key = (cs, ip)
            if key in decoded:
                break
            linear = cs * 16 + ip
            if linear < 0 or linear >= len(module):
                break
            candidates = list(md.disasm(module[linear : linear + 15], ip, count=1))
            if not candidates:
                break
            insn = candidates[0]
            if not insn.size or linear + insn.size > len(module):
                break

            record = DecodedInstruction(
                cs=cs,
                ip=ip,
                linear=linear,
                size=insn.size,
                raw=bytes(insn.bytes),
                mnemonic=insn.mnemonic,
                operands=insn.op_str,
            )
            decoded[key] = record

            raw = record.raw
            source = f"{cs:04X}:{ip:04X}"
            is_call = CS_GRP_CALL in insn.groups
            is_jump = CS_GRP_JUMP in insn.groups
            is_return = CS_GRP_RET in insn.groups or insn.mnemonic in {"retf", "iret", "iretd"}
            direct_target: tuple[int, int] | None = None
            edge_kind: str | None = None

            if len(raw) >= 5 and raw[0] in (0x9A, 0xEA):
                target_ip, target_cs = struct.unpack_from("<HH", raw, 1)
                direct_target = (target_cs, target_ip)
                edge_kind = "far_call" if raw[0] == 0x9A else "far_jump"
            elif (is_call or is_jump) and insn.operands and insn.operands[0].type == X86_OP_IMM:
                direct_target = (cs, int(insn.operands[0].imm) & 0xFFFF)
                edge_kind = "near_call" if is_call else "near_jump"

            if direct_target is not None:
                target_cs, target_ip = direct_target
                target_linear = target_cs * 16 + target_ip
                if 0 <= target_linear < len(module):
                    edges.append(
                        {
                            "kind": edge_kind or "branch",
                            "source_segment": cs,
                            "source_offset": ip,
                            "source_linear": linear,
                            "target_segment": target_cs,
                            "target_offset": target_ip,
                            "target_linear": target_linear,
                        }
                    )
                    enqueue(target_cs, target_ip, f"{edge_kind} from {source}")

            next_ip = (ip + insn.size) & 0xFFFF
            wrapped = next_ip < ip
            unconditional_jump = is_jump and insn.mnemonic in {"jmp", "ljmp"}
            if is_return or unconditional_jump or insn.mnemonic in {"hlt", "ud2"} or wrapped:
                break
            ip = next_ip

    return decoded, labels, edges


def choose_physical_instructions(
    decoded: dict[tuple[int, int], DecodedInstruction], entry: tuple[int, int]
) -> tuple[dict[int, DecodedInstruction], list[dict[str, int]]]:
    """Choose a non-overlapping physical representation for the byte map."""
    ordered = sorted(
        decoded.values(),
        key=lambda ins: (0 if (ins.cs, ins.ip) == entry else 1, ins.linear, ins.cs, ins.ip),
    )
    selected: dict[int, DecodedInstruction] = {}
    owners: dict[int, int] = {}
    conflicts: list[dict[str, int]] = []
    for ins in ordered:
        claimed = range(ins.linear, ins.linear + ins.size)
        if any(byte in owners for byte in claimed):
            conflicts.append({"segment": ins.cs, "offset": ins.ip, "linear": ins.linear, "size": ins.size})
            continue
        selected[ins.linear] = ins
        for byte in claimed:
            owners[byte] = ins.linear
    return selected, conflicts


def write_byte_map(
    path: Path,
    module: bytes,
    selected: dict[int, DecodedInstruction],
    labels: dict[tuple[int, int], set[str]],
) -> None:
    labels_by_linear: dict[int, list[tuple[int, int, str]]] = {}
    for (cs, ip), reasons in labels.items():
        linear = cs * 16 + ip
        if 0 <= linear < len(module):
            labels_by_linear.setdefault(linear, []).append((cs, ip, "; ".join(sorted(reasons))))

    lines = [
        "; Ken's Labyrinth v2.1 KEN.EXE relocation-aware byte map",
        "; All load-module bytes occur exactly once. Instructions are shown as",
        "; comments beside exact DB bytes so this file is losslessly assemblable.",
        "bits 16",
        "org 0",
        "",
    ]
    cursor = 0
    while cursor < len(module):
        if cursor in labels_by_linear:
            for cs, ip, reason in labels_by_linear[cursor]:
                lines.append(f"loc_{cs:04x}_{ip:04x}: ; module+0x{cursor:05X} - {reason}")

        ins = selected.get(cursor)
        if ins is not None:
            byte_text = ", ".join(f"0x{value:02X}" for value in ins.raw)
            decoded_text = f"{ins.mnemonic} {ins.operands}".rstrip()
            lines.append(
                f"    db {byte_text:<35} ; {ins.cs:04X}:{ins.ip:04X}  {decoded_text}"
            )
            cursor += ins.size
            continue

        start = cursor
        while cursor < len(module) and cursor not in selected and cursor not in labels_by_linear and cursor - start < 16:
            cursor += 1
        if cursor == start:
            cursor += 1
        raw = module[start:cursor]
        byte_text = ", ".join(f"0x{value:02X}" for value in raw)
        lines.append(f"    db {byte_text:<79} ; module+0x{start:05X} unclassified/data")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_readme(path: Path, exe_name: str, header: dict[str, int], coverage: dict[str, int | str]) -> None:
    text = f"""# Exact DOS executable disassembly

This directory analyzes `{exe_name}` byte-for-byte. Its SHA-256 is
`{coverage['executable_sha256']}`. The executable is a 16-bit DOS MZ built with
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
plus the 0x{header['header_size']:X}-byte MZ header. DOS adds the process load
segment to each word listed in `relocations.csv`.

The entry point is `{header['e_cs']:04X}:{header['e_ip']:04X}` (module
`0x{header['entry_linear']:05X}`, file `0x{header['entry_file_offset']:05X}`).

## Reproduce

From the repository root:

```powershell
python tools/disassemble_mz.py references/official-full/KEN.EXE disassembly
```
"""
    path.write_text(text, encoding="utf-8")


def write_source_function_index(path: Path, source_dir: Path) -> None:
    """Index the original source's top-level K&R/ANSI function definitions."""
    signature_pattern = re.compile(r"^[A-Za-z_][A-Za-z0-9_ *]*\([^;{}]*\)\s*$")
    name_pattern = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    records: list[tuple[str, int, str, str]] = []
    for source in sorted(source_dir.glob("*.C")):
        for line_number, line in enumerate(source.read_text(encoding="cp437").splitlines(), 1):
            if line != line.lstrip():
                continue
            if not signature_pattern.match(line.rstrip()):
                continue
            name_match = name_pattern.search(line)
            if not name_match:
                continue
            name = name_match.group(1)
            if name in {"if", "for", "while", "switch"}:
                continue
            records.append((source.name, line_number, name, line.rstrip()))

    width = max((len(record[2]) for record in records), default=8)
    lines = [
        "Official Ken's Labyrinth v2.1 source function index",
        "Generated from top-level definitions in the released C files.",
        "",
    ]
    for filename, line_number, name, signature in records:
        lines.append(f"{name:<{width}}  {filename}:{line_number:<5}  {signature}")
    lines.append("")
    lines.append(f"Total: {len(records)} functions")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    exe = args.executable.read_bytes()
    header, module, relocations = parse_mz(exe)
    args.output.mkdir(parents=True, exist_ok=True)

    module_path = args.output / "ken_load_module.bin"
    module_path.write_bytes(module)

    header_report = {
        **{key: ("MZ" if key == "e_magic" else value) for key, value in header.items()},
        "executable_sha256": sha256(exe),
        "load_module_sha256": sha256(module),
    }
    (args.output / "mz_header.json").write_text(
        json.dumps(header_report, indent=2) + "\n", encoding="utf-8"
    )

    with (args.output / "relocations.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=("index", "site_segment", "site_offset", "module_offset", "file_offset"),
        )
        writer.writeheader()
        for relocation in relocations:
            writer.writerow(
                {
                    "index": relocation.index,
                    "site_segment": f"0x{relocation.segment:04X}",
                    "site_offset": f"0x{relocation.offset:04X}",
                    "module_offset": f"0x{relocation.linear:05X}",
                    "file_offset": f"0x{header['header_size'] + relocation.linear:05X}",
                }
            )

    far_targets = fixed_far_targets(module, relocations)
    with (args.output / "fixed_far_targets.csv").open("w", newline="", encoding="utf-8") as handle:
        fields = tuple(far_targets[0].keys()) if far_targets else ("kind",)
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(far_targets)

    entry = (header["e_cs"], header["e_ip"])
    decoded, labels, edges = traverse(module, entry, far_targets)
    selected, conflicts = choose_physical_instructions(decoded, entry)

    with (args.output / "control_flow_edges.csv").open("w", newline="", encoding="utf-8") as handle:
        fields = tuple(edges[0].keys()) if edges else ("kind",)
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(edges)

    functions = []
    for (cs, ip), reasons in sorted(labels.items(), key=lambda item: item[0][0] * 16 + item[0][1]):
        linear = cs * 16 + ip
        if linear >= len(module):
            continue
        functions.append(
            {
                "label": f"loc_{cs:04x}_{ip:04x}",
                "segment": cs,
                "offset": ip,
                "module_offset": linear,
                "file_offset": header["header_size"] + linear,
                "reasons": sorted(reasons),
            }
        )
    (args.output / "discovered-entry-points.json").write_text(
        json.dumps(functions, indent=2) + "\n", encoding="utf-8"
    )

    write_byte_map(args.output / "KEN.EXE.byte-map.asm", module, selected, labels)

    roundtrip_verified = False
    nasm = shutil.which("nasm")
    if nasm:
        with tempfile.TemporaryDirectory() as temporary:
            roundtrip_path = Path(temporary) / "ken_load_module.bin"
            subprocess.run(
                [nasm, "-f", "bin", str(args.output / "KEN.EXE.byte-map.asm"), "-o", str(roundtrip_path)],
                check=True,
                capture_output=True,
            )
            if roundtrip_path.read_bytes() != module:
                raise RuntimeError("byte-map assembly did not reproduce the original load module")
            roundtrip_verified = True

    ndisasm = shutil.which("ndisasm")
    if not ndisasm:
        raise RuntimeError("ndisasm was not found on PATH")
    completed = subprocess.run(
        [ndisasm, "-a", "-b", "16", "-o", "0", str(module_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    ndisasm_header = (
        "; Exhaustive flat decode of the complete KEN.EXE load module\n"
        "; Addresses are module-linear, beginning after the MZ header.\n"
        "; Data is intentionally decoded too; use KEN.EXE.byte-map.asm for classification.\n\n"
    )
    (args.output / "KEN.EXE.ndisasm.asm").write_text(
        ndisasm_header + completed.stdout, encoding="utf-8"
    )

    code_bytes: set[int] = set()
    for ins in selected.values():
        code_bytes.update(range(ins.linear, ins.linear + ins.size))
    coverage: dict[str, int | str | bool] = {
        "executable_sha256": sha256(exe),
        "load_module_sha256": sha256(module),
        "load_module_bytes": len(module),
        "byte_map_bytes_emitted": len(module),
        "byte_map_coverage_percent": 100,
        "byte_map_roundtrip_verified": roundtrip_verified,
        "classified_code_bytes": len(code_bytes),
        "classified_code_percent": round(len(code_bytes) * 100 / len(module), 2),
        "unclassified_or_data_bytes": len(module) - len(code_bytes),
        "logical_instructions": len(decoded),
        "selected_physical_instructions": len(selected),
        "overlap_conflicts": len(conflicts),
        "discovered_entry_points": len(functions),
        "control_flow_edges": len(edges),
        "fixed_far_targets": len(far_targets),
        "mz_relocations": len(relocations),
    }
    (args.output / "coverage.json").write_text(
        json.dumps(coverage, indent=2) + "\n", encoding="utf-8"
    )
    (args.output / "overlap-conflicts.json").write_text(
        json.dumps(conflicts, indent=2) + "\n", encoding="utf-8"
    )

    source_dir = Path("references/original-source")
    if source_dir.is_dir():
        write_source_function_index(args.output / "source-functions.txt", source_dir)

    write_readme(args.output / "README.md", args.executable.name, header, coverage)
    print(json.dumps(coverage, indent=2))


if __name__ == "__main__":
    main()
