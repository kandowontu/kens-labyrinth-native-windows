#!/usr/bin/env python3
"""Convert the decompressed Ken's Labyrinth v2.1 data into common formats.

The input directory is expected to have been expanded with KKIT/CLI first.  This
script deliberately keeps the lossless DAT/KSM files intact and creates a
second, convenient view: indexed PNG graphics, JSON maps and instruments,
plain-text story data, MIDI previews, and metadata manifests.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import wave
from pathlib import Path

from PIL import Image, ImageDraw


WALL_COUNT = 448
WALL_SIZE = 64
WALL_DATA_OFFSET = 1024
BOARD_COUNT = 30
BOARD_SIZE = 64
BOARD_BYTES = BOARD_SIZE * BOARD_SIZE * 2
TRANSPARENT_INDEX = 255
FLOOR_RGB = (72, 56, 28)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def write_json(path: Path, value: object, *, compact: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        if compact:
            json.dump(value, stream, ensure_ascii=False, separators=(",", ":"))
        else:
            json.dump(value, stream, ensure_ascii=False, indent=2)
            stream.write("\n")


def parse_wall_names(header_path: Path | None) -> dict[int, str]:
    if header_path is None or not header_path.exists():
        return {}
    text = header_path.read_text(encoding="cp437")
    match = re.search(r"//SORTWALLSTART(.*?)//SORTWALLEND", text, re.DOTALL)
    if not match:
        return {}
    return {
        int(number): name
        for name, number in re.findall(r"^#define\s+([A-Za-z_]\w*)\s+(\d+)", match.group(1), re.MULTILINE)
    }


def load_palette(input_dir: Path) -> list[int]:
    palette_gif = input_dir / "gif" / "LAB3D-002.gif"
    with Image.open(palette_gif) as image:
        palette = list(image.getpalette() or [])
    if len(palette) != 768:
        raise ValueError(f"Expected a 256-colour palette in {palette_gif}")
    return palette


def save_palette(palette: list[int], output_dir: Path) -> None:
    image = Image.new("RGB", (256, 256))
    draw = ImageDraw.Draw(image)
    for index in range(256):
        x = (index % 16) * 16
        y = (index // 16) * 16
        draw.rectangle((x, y, x + 15, y + 15), fill=tuple(palette[index * 3:index * 3 + 3]))
    image.save(output_dir / "palette.png", optimize=True)
    write_json(
        output_dir / "palette.json",
        [{"index": i, "rgb": palette[i * 3:i * 3 + 3]} for i in range(256)],
    )


def indexed_wall_image(raw: bytes, palette: list[int]) -> Image.Image:
    # WALLS.DAT is column-major: byte x*64+y. PNG scanlines are row-major.
    row_major = bytes(raw[x * WALL_SIZE + y] for y in range(WALL_SIZE) for x in range(WALL_SIZE))
    image = Image.frombytes("P", (WALL_SIZE, WALL_SIZE), row_major)
    image.putpalette(palette)
    image.info["transparency"] = TRANSPARENT_INDEX
    return image


def export_walls(input_dir: Path, output_dir: Path, palette: list[int], names: dict[int, str]) -> tuple[list[Image.Image], list[dict[str, object]]]:
    data = (input_dir / "WALLS.DAT").read_bytes()
    expected = WALL_DATA_OFFSET + WALL_COUNT * WALL_SIZE * WALL_SIZE
    if len(data) != expected:
        raise ValueError(f"WALLS.DAT is {len(data)} bytes; expected {expected}")

    wall_dir = output_dir / "walls"
    wall_dir.mkdir(parents=True, exist_ok=True)
    images: list[Image.Image] = []
    manifest: list[dict[str, object]] = []
    type_names = {0: "cube", 1: "plane", 2: "directional"}

    for index in range(WALL_COUNT):
        game_id = index + 1
        flags = data[index]
        kind = flags & 7
        raw = data[WALL_DATA_OFFSET + index * 4096:WALL_DATA_OFFSET + (index + 1) * 4096]
        image = indexed_wall_image(raw, palette)
        label = names.get(game_id)
        suffix = f"_{label}" if label else ""
        filename = f"wall_{game_id:03d}{suffix}.png"
        image.save(wall_dir / filename, optimize=True, transparency=TRANSPARENT_INDEX)
        images.append(image.convert("RGBA"))
        manifest.append(
            {
                "index": index,
                "game_id": game_id,
                "name": label,
                "file": f"walls/{filename}",
                "header_byte": flags,
                "type": type_names.get(kind, f"unknown-{kind}"),
                "inside_or_noclip": bool(flags & 8),
                "destructible": bool(flags & 16),
                "raw_sha256": hashlib.sha256(raw).hexdigest().upper(),
            }
        )

    sheet = Image.new("RGBA", (16 * WALL_SIZE, 28 * WALL_SIZE), (204, 92, 112, 0))
    for index, image in enumerate(images):
        sheet.alpha_composite(image, ((index % 16) * WALL_SIZE, (index // 16) * WALL_SIZE))
    sheet.save(output_dir / "wall_sheet.png", optimize=True)
    write_json(output_dir / "walls.json", manifest)
    return images, manifest


def decode_board_value(value: int) -> dict[str, object]:
    player = bool(value & 0x1000)
    wall_index = (value & 0x03FF) - 1
    direction = None
    if player:
        direction = ("right", "down", "left", "up")[value & 3]
        wall_index = -1
    return {
        "raw": value,
        "wall_index": wall_index,
        "game_wall_id": wall_index + 1 if wall_index >= 0 else 0,
        "inside_or_noclip": bool(value & 0x0400),
        "destructible": bool(value & 0x0800),
        "player_start": player,
        "vertical": bool(value & 0x2000),
        "bit_14": bool(value & 0x4000),
        "bit_15": bool(value & 0x8000),
        "player_direction": direction,
    }


def draw_player(draw: ImageDraw.ImageDraw, x: int, y: int, direction: str, scale: int) -> None:
    left, top = x * scale, y * scale
    right, bottom = left + scale - 1, top + scale - 1
    cx, cy = left + scale // 2, top + scale // 2
    points = {
        "right": ((right, cy), (left, top), (left, bottom)),
        "down": ((cx, bottom), (left, top), (right, top)),
        "left": ((left, cy), (right, top), (right, bottom)),
        "up": ((cx, top), (left, bottom), (right, bottom)),
    }[direction]
    draw.polygon(points, fill=(255, 40, 40, 255), outline=(255, 255, 255, 255))


def export_boards(input_dir: Path, output_dir: Path, walls: list[Image.Image]) -> list[dict[str, object]]:
    data = (input_dir / "BOARDS.DAT").read_bytes()
    if len(data) != BOARD_COUNT * BOARD_BYTES:
        raise ValueError(f"BOARDS.DAT is {len(data)} bytes; expected {BOARD_COUNT * BOARD_BYTES}")

    board_dir = output_dir / "boards"
    board_dir.mkdir(parents=True, exist_ok=True)
    thumb_scale = 8
    thumbs = [image.resize((thumb_scale, thumb_scale), Image.Resampling.NEAREST) for image in walls]
    boards_manifest: list[dict[str, object]] = []

    for board_index in range(BOARD_COUNT):
        raw = data[board_index * BOARD_BYTES:(board_index + 1) * BOARD_BYTES]
        rows: list[list[dict[str, object]]] = []
        player: dict[str, object] | None = None
        overview = Image.new("RGBA", (BOARD_SIZE * thumb_scale, BOARD_SIZE * thumb_scale), FLOOR_RGB + (255,))

        # The original layout is x-major (board[x][y]), matching the game source.
        decoded_by_xy: list[list[dict[str, object]]] = [[{} for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                offset = 2 * (x * BOARD_SIZE + y)
                value = struct.unpack_from("<H", raw, offset)[0]
                tile = decode_board_value(value)
                decoded_by_xy[x][y] = tile
                wall_index = int(tile["wall_index"])
                if 0 <= wall_index < len(thumbs):
                    overview.alpha_composite(thumbs[wall_index], (x * thumb_scale, y * thumb_scale))
                if tile["player_start"]:
                    player = {"x": x, "y": y, "direction": tile["player_direction"]}

        # JSON rows are y-major for conventional map consumers.
        for y in range(BOARD_SIZE):
            rows.append([decoded_by_xy[x][y] for x in range(BOARD_SIZE)])
        if player:
            draw_player(ImageDraw.Draw(overview), int(player["x"]), int(player["y"]), str(player["direction"]), thumb_scale)

        board_number = board_index + 1
        png_name = f"board_{board_number:02d}.png"
        json_name = f"board_{board_number:02d}.json"
        overview.save(board_dir / png_name, optimize=True)
        write_json(
            board_dir / json_name,
            {
                "board_index": board_index,
                "board_number": board_number,
                "episode": board_index // 10 + 1,
                "level_in_episode": board_index % 10 + 1,
                "width": BOARD_SIZE,
                "height": BOARD_SIZE,
                "source_layout": "x-major little-endian uint16",
                "json_layout": "rows[y][x]",
                "player_start": player,
                "rows": rows,
            },
            compact=True,
        )
        boards_manifest.append(
            {
                "board_index": board_index,
                "board_number": board_number,
                "episode": board_index // 10 + 1,
                "level_in_episode": board_index % 10 + 1,
                "player_start": player,
                "image": f"boards/{png_name}",
                "data": f"boards/{json_name}",
                "raw_sha256": hashlib.sha256(raw).hexdigest().upper(),
            }
        )

    write_json(output_dir / "boards.json", boards_manifest)
    return boards_manifest


def encode_vlq(value: int) -> bytes:
    result = [value & 0x7F]
    value >>= 7
    while value:
        result.append(0x80 | (value & 0x7F))
        value >>= 7
    return bytes(reversed(result))


def midi_track(events: list[tuple[int, int, bytes]]) -> bytes:
    events.sort(key=lambda event: (event[0], event[1]))
    output = bytearray()
    last_tick = 0
    for tick, _priority, message in events:
        output.extend(encode_vlq(max(0, tick - last_tick)))
        output.extend(message)
        last_tick = tick
    output.extend(b"\x00\xFF\x2F\x00")
    return b"MTrk" + struct.pack(">I", len(output)) + output


def ksm_to_midi(ksm: bytes, gm_map: list[int]) -> tuple[bytes, dict[str, object]]:
    if len(ksm) < 82:
        raise ValueError("KSM file is too short")
    trinst = list(ksm[0:16])
    trquant = list(ksm[16:32])
    trchan = list(ksm[32:48])
    trprio = list(ksm[48:64])
    trvol = list(ksm[64:80])
    note_count = struct.unpack_from("<H", ksm, 80)[0]
    used_length = 82 + note_count * 4
    if len(ksm) < used_length:
        raise ValueError(f"KSM note count implies at least {used_length} bytes, got {len(ksm)}")

    events: list[tuple[int, int, bytes]] = [(0, 0, b"\xFF\x51\x03\x0F\x42\x40")]  # 1,000,000 us/qn; 240 ticks/s.
    channels = [0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11]
    for track, channel in enumerate(channels):
        program = gm_map[trinst[track]] if trinst[track] < len(gm_map) else 0
        events.append((0, 1, bytes((0xC0 | channel, program & 0x7F))))
        volume = min(127, round(trvol[track] * 127 / 63))
        events.append((0, 2, bytes((0xB0 | channel, 7, volume))))

    drum_notes = {11: 36, 12: 38, 13: 45, 14: 49, 15: 42}
    max_tick = 0
    for offset in range(82, used_length, 4):
        value = struct.unpack_from("<I", ksm, offset)[0]
        note = value & 0x3F
        note_on = bool(value & 0x40)
        track = (value >> 8) & 0x0F
        tick = value >> 12
        max_tick = max(max_tick, tick)
        if track >= 11:
            midi_note = drum_notes[track]
            events.append((tick, 4, bytes((0x99, midi_note, 110))))
            events.append((tick + 12, 3, bytes((0x89, midi_note, 0))))
        else:
            channel = channels[track]
            midi_note = max(0, min(127, 35 + note))
            status = (0x90 if note_on else 0x80) | channel
            velocity = 100 if note_on else 0
            events.append((tick, 4 if note_on else 3, bytes((status, midi_note, velocity))))

    track_data = midi_track(events)
    midi = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 240) + track_data
    metadata = {
        "note_event_count": note_count,
        "trailing_archive_padding_bytes": len(ksm) - used_length,
        "duration_ticks": max_tick,
        "duration_seconds": round(max_tick / 240, 3),
        "track_instruments": trinst,
        "track_quantization": trquant,
        "track_channel_counts": trchan,
        "track_priorities": trprio,
        "track_volumes": trvol,
    }
    return midi, metadata


def export_music(input_dir: Path, output_dir: Path, gm_map_path: Path | None) -> list[dict[str, object]]:
    gm_map = [0] * 256
    if gm_map_path and gm_map_path.exists():
        values = [int(line.strip()) for line in gm_map_path.read_text(encoding="ascii").splitlines() if line.strip()]
        gm_map[:min(256, len(values))] = values[:256]
    midi_dir = output_dir / "midi"
    midi_dir.mkdir(parents=True, exist_ok=True)
    manifest: list[dict[str, object]] = []
    for path in sorted((input_dir / "KSM").glob("*.KSM")):
        data = path.read_bytes()
        midi, metadata = ksm_to_midi(data, gm_map)
        midi_path = midi_dir / f"{path.stem}.mid"
        midi_path.write_bytes(midi)
        manifest.append(
            {
                "name": path.stem,
                "ksm_file": f"KSM/{path.name}",
                "midi_file": f"midi/{midi_path.name}",
                "ksm_sha256": hashlib.sha256(data).hexdigest().upper(),
                **metadata,
            }
        )
    write_json(output_dir / "music.json", manifest)
    return manifest


def export_instruments(input_dir: Path, output_dir: Path, gm_map_path: Path | None) -> list[dict[str, object]]:
    data = (input_dir / "insts.dat").read_bytes()
    if len(data) % 33:
        raise ValueError("INSTS.DAT is not a whole number of 33-byte records")
    gm_map: list[int] = []
    if gm_map_path and gm_map_path.exists():
        gm_map = [int(line.strip()) for line in gm_map_path.read_text(encoding="ascii").splitlines() if line.strip()]
    result = []
    for index in range(len(data) // 33):
        record = data[index * 33:(index + 1) * 33]
        result.append(
            {
                "index": index,
                "name": record[:20].split(b"\0", 1)[0].decode("cp437"),
                "opl2_register_data": list(record[20:31]),
                "gm_program": gm_map[index] if index < len(gm_map) else 0,
            }
        )
    write_json(output_dir / "instruments.json", result)
    return result


def export_story(input_dir: Path, output_dir: Path) -> dict[str, object]:
    data = (input_dir / "STORY.DAT").read_bytes()
    text = data.decode("cp437")
    (output_dir / "story.txt").write_text(text, encoding="utf-8", newline="\n")
    sections = [section.strip("\r\n") for section in text.split("\\")]
    value = {"encoding": "CP437", "delimiter": "backslash", "sections": sections}
    write_json(output_dir / "story.json", value)
    return {"section_count": len(sections), "sha256": hashlib.sha256(data).hexdigest().upper()}


def export_gifs(input_dir: Path, output_dir: Path) -> list[dict[str, object]]:
    png_dir = output_dir / "screens"
    png_dir.mkdir(parents=True, exist_ok=True)
    result = []
    for path in sorted((input_dir / "gif").glob("*.gif")):
        with Image.open(path) as source:
            frame = source.convert("RGBA")
            png_path = png_dir / f"{path.stem}.png"
            frame.save(png_path, optimize=True)
            result.append(
                {
                    "gif_file": f"gif/{path.name}",
                    "png_file": f"screens/{png_path.name}",
                    "width": source.width,
                    "height": source.height,
                    "gif_sha256": sha256(path),
                }
            )
    write_json(output_dir / "screens.json", result)
    return result


def export_sounds(input_dir: Path, output_dir: Path) -> list[dict[str, object]]:
    result = []
    for path in sorted((input_dir / "wav").glob("*.wav")):
        with wave.open(str(path), "rb") as stream:
            metadata = {
                "channels": stream.getnchannels(),
                "sample_width_bits": stream.getsampwidth() * 8,
                "sample_rate": stream.getframerate(),
                "frame_count": stream.getnframes(),
                "duration_seconds": round(stream.getnframes() / stream.getframerate(), 6),
            }
        result.append({"file": f"wav/{path.name}", "sha256": sha256(path), **metadata})
    write_json(output_dir / "sounds.json", result)
    return result


def copy_lossless_inputs(input_dir: Path, output_dir: Path) -> None:
    for subdir in ("gif", "KSM", "wav"):
        destination = output_dir / subdir
        shutil.copytree(input_dir / subdir, destination, dirs_exist_ok=True)
    for filename in ("BOARDS.DAT", "WALLS.DAT", "STORY.DAT", "insts.dat", "tables.dat"):
        shutil.copy2(input_dir / filename, output_dir / filename)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="KKIT/CLI-expanded v2.1 data directory")
    parser.add_argument("output", type=Path, help="output asset directory")
    parser.add_argument("--source-header", type=Path, help="original LAB3D.H, used for texture names")
    parser.add_argument("--gm-map", type=Path, help="LAB3D/SDL ksmmidi.txt instrument mapping")
    args = parser.parse_args()

    input_dir = args.input.resolve()
    output_dir = args.output.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    copy_lossless_inputs(input_dir, output_dir)

    palette = load_palette(input_dir)
    save_palette(palette, output_dir)
    wall_names = parse_wall_names(args.source_header)
    walls, wall_manifest = export_walls(input_dir, output_dir, palette, wall_names)
    boards = export_boards(input_dir, output_dir, walls)
    music = export_music(input_dir, output_dir, args.gm_map)
    instruments = export_instruments(input_dir, output_dir, args.gm_map)
    story = export_story(input_dir, output_dir)
    screens = export_gifs(input_dir, output_dir)
    sounds = export_sounds(input_dir, output_dir)

    source_files = []
    for path in sorted(input_dir.rglob("*")):
        if path.is_file():
            source_files.append(
                {
                    "file": path.relative_to(input_dir).as_posix(),
                    "bytes": path.stat().st_size,
                    "sha256": sha256(path),
                }
            )
    manifest = {
        "game": "Ken's Labyrinth",
        "version": "2.1 registered",
        "wall_count": len(wall_manifest),
        "board_count": len(boards),
        "song_count": len(music),
        "instrument_count": len(instruments),
        "sound_count": len(sounds),
        "screen_count": len(screens),
        "story": story,
        "source_files": source_files,
    }
    write_json(output_dir / "manifest.json", manifest)
    print(json.dumps({key: manifest[key] for key in manifest if key.endswith("_count")}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
