#!/usr/bin/env python3
"""Hash the preserved source VHD and every extracted installation file."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def describe(path: Path, relative_to: Path | None = None) -> dict[str, object]:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return {
        "file": path.relative_to(relative_to).as_posix() if relative_to else path.name,
        "bytes": path.stat().st_size,
        "sha256": digest.hexdigest().upper(),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("vhd", type=Path)
    parser.add_argument("game_directory", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    files = [describe(path, args.game_directory) for path in sorted(args.game_directory.rglob("*")) if path.is_file()]
    report = {
        "original_network_path": r"\\192.168.1.176\sdcard\games\AO486\media\Ken's Labyrinth\kens.vhd",
        "vhd": describe(args.vhd),
        "extraction": {
            "filesystem": "FAT16 LBA",
            "method": "read-only mount",
            "file_count": len(files),
            "total_file_bytes": sum(int(item["bytes"]) for item in files),
            "files": files,
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
