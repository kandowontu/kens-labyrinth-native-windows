from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import tempfile
import unittest
import wave
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
EXE_SHA256 = "6D33F1502C80CDF2307B86AE77662F478F3C537367631043489442D373BB75C1"
MODULE_SHA256 = "DD3D9A51E5C595E6B781685AF57F400F0CB4F927BB8B369C7B7118BBE37E35CB"


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


class ExactBinaryTests(unittest.TestCase):
    def test_preserved_vhd_inventory(self) -> None:
        inventory = json.loads((ROOT / "original" / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(inventory["vhd"]["bytes"], 33_546_752)
        self.assertEqual(
            inventory["vhd"]["sha256"],
            "72FB5B5856169FF43F426BC362ACBEE0E6A49E8BED74FFBE24489B2D827B5F99",
        )
        self.assertEqual(inventory["extraction"]["file_count"], 18)

    def test_original_executable_identity(self) -> None:
        executable = ROOT / "original" / "Kens" / "KEN.EXE"
        self.assertEqual(executable.stat().st_size, 110_775)
        self.assertEqual(file_sha256(executable), EXE_SHA256)

    def test_disassembly_reports_full_byte_coverage(self) -> None:
        coverage = json.loads((ROOT / "disassembly" / "coverage.json").read_text())
        self.assertEqual(coverage["executable_sha256"], EXE_SHA256)
        self.assertEqual(coverage["load_module_sha256"], MODULE_SHA256)
        self.assertEqual(coverage["load_module_bytes"], 105_143)
        self.assertEqual(coverage["byte_map_bytes_emitted"], 105_143)
        self.assertEqual(coverage["byte_map_coverage_percent"], 100)
        self.assertEqual(coverage["mz_relocations"], 1_349)
        self.assertGreater(coverage["classified_code_percent"], 90)

    def test_byte_map_round_trips(self) -> None:
        nasm = shutil.which("nasm")
        if not nasm:
            self.skipTest("nasm is not on PATH")
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "module.bin"
            subprocess.run(
                [nasm, "-f", "bin", str(ROOT / "disassembly" / "KEN.EXE.byte-map.asm"), "-o", str(output)],
                check=True,
                capture_output=True,
            )
            self.assertEqual(file_sha256(output), MODULE_SHA256)


class ConvertedAssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.assets = ROOT / "assets" / "converted" / "v2.1"
        cls.manifest = json.loads((cls.assets / "manifest.json").read_text(encoding="utf-8"))

    def test_manifest_counts(self) -> None:
        expected = {
            "wall_count": 448,
            "board_count": 30,
            "song_count": 39,
            "instrument_count": 256,
            "sound_count": 33,
            "screen_count": 3,
        }
        for key, value in expected.items():
            self.assertEqual(self.manifest[key], value)
        self.assertEqual(self.manifest["story"]["section_count"], 65)

    def test_graphics_dimensions_and_palette(self) -> None:
        walls = sorted((self.assets / "walls").glob("*.png"))
        boards = sorted((self.assets / "boards").glob("*.png"))
        screens = sorted((self.assets / "screens").glob("*.png"))
        self.assertEqual((len(walls), len(boards), len(screens)), (448, 30, 3))
        with Image.open(walls[0]) as image:
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(image.mode, "P")
            self.assertEqual(image.info.get("transparency"), 255)
        with Image.open(boards[0]) as image:
            self.assertEqual(image.size, (512, 512))
        with Image.open(self.assets / "wall_sheet.png") as image:
            self.assertEqual(image.size, (1024, 1792))

    def test_board_json_layout(self) -> None:
        board = json.loads((self.assets / "boards" / "board_01.json").read_text(encoding="utf-8"))
        self.assertEqual(board["source_layout"], "x-major little-endian uint16")
        self.assertEqual(board["json_layout"], "rows[y][x]")
        self.assertEqual(len(board["rows"]), 64)
        self.assertTrue(all(len(row) == 64 for row in board["rows"]))
        self.assertEqual(board["player_start"], {"x": 24, "y": 38, "direction": "up"})

    def test_music_and_sound_are_standard_files(self) -> None:
        midi = sorted((self.assets / "midi").glob("*.mid"))
        wav = sorted((self.assets / "wav").glob("*.wav"))
        self.assertEqual((len(midi), len(wav)), (39, 33))
        self.assertTrue(all(path.read_bytes().startswith(b"MThd") for path in midi))
        for path in wav:
            with wave.open(str(path), "rb") as stream:
                self.assertGreater(stream.getnframes(), 0)
                self.assertGreater(stream.getframerate(), 0)

    def test_lossless_expanded_data_sizes(self) -> None:
        self.assertEqual((self.assets / "WALLS.DAT").stat().st_size, 1_836_032)
        self.assertEqual((self.assets / "BOARDS.DAT").stat().st_size, 245_760)
        self.assertEqual((self.assets / "insts.dat").stat().st_size, 8_448)
        story = json.loads((self.assets / "story.json").read_text(encoding="utf-8"))
        self.assertEqual(story["encoding"], "CP437")
        self.assertEqual(len(story["sections"]), 65)


if __name__ == "__main__":
    unittest.main()
