from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class CheatMenuSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.main = (ROOT / "src" / "lab3d" / "lab3d.c").read_text(encoding="utf-8")
        cls.subs = (ROOT / "src" / "lab3d" / "subs.c").read_text(encoding="utf-8")
        cls.header = (ROOT / "src" / "lab3d" / "lab3d.h").read_text(encoding="utf-8")

    def test_ctrl_alt_f1_contract(self) -> None:
        self.assertIn("cheat_menu_requested", self.main)
        self.assertIn("event.key.keysym.mod&KMOD_CTRL", self.subs)
        self.assertIn("event.key.keysym.mod&KMOD_ALT", self.subs)
        self.assertIn("cheatf1down", self.subs)
        self.assertIn("ingamecheatmenu();", self.main)
        self.assertNotIn("cheatenable && ingamecheatmenu", self.main)

    def test_cheat_menu_uses_compositor_safe_presentation(self) -> None:
        menu = self.subs[self.subs.index("static K_INT16 getcheatselection") :]
        menu = menu[: menu.index("/* Ctrl+Alt+F1 cheat menu")]
        self.assertIn("gfxDrawBack();", menu)
        self.assertIn("gfxSwapBuffers();", menu)
        self.assertNotIn("gfxDrawFront();", menu)

    def test_cheat_menu_heading_stays_inside_frame(self) -> None:
        self.assertIn("drawmenu(304,224,menu)", self.subs)
        self.assertIn('"Cheat menu"', self.subs)
        self.assertNotIn('"Cheat menu - Ctrl+Alt+F1"', self.subs)

    def test_menu_exposes_every_requested_group(self) -> None:
        labels = (
            "Toggle all",
            "God mode",
            "Noclip",
            "Money refill",
            "Key refill",
            "Max weapons",
            "Powerup refill",
            "Life refill",
            "Compass",
            "Restore full health",
            "Next board",
            "Chapter %d: %s",
            "Level %d of %d",
            "Warp to Chapter %d, Level %d",
            "Return to game",
        )
        for label in labels:
            self.assertIn(label, self.subs)
        self.assertIn("getcheatselection(selection,n,&chapter,&level)", self.subs)

    def test_level_select_covers_every_installed_board(self) -> None:
        self.assertIn("(numboards+9)/10", self.subs)
        self.assertIn("numboards-(chapter*10)", self.subs)
        self.assertIn("cheat_warp_board = chapter*10+level", self.subs)
        self.assertIn("(cheat_warp_board >= 0)", self.main)
        self.assertIn("boardnum = cheatwarp", self.main)

    def test_persistent_cheats_mark_the_run_cheated(self) -> None:
        self.assertRegex(
            self.subs,
            re.compile(r"cheat_godmode \|\| cheat_noclip.*?\(cheated == 0\).*?cheated = 1", re.S),
        )

    def test_god_mode_guards_all_player_hazards(self) -> None:
        self.assertGreaterEqual(self.main.count("!cheat_godmode"), 4)
        self.assertIn("life = 4095;", self.subs)
        self.assertIn("death = 4095;", self.subs)

    def test_noclip_uses_bounded_direct_movement(self) -> None:
        self.assertIn("clamp_noclip_position", self.main)
        self.assertGreaterEqual(self.main.count("if (cheat_noclip)"), 2)
        self.assertIn("if (position < 1024)", self.main)
        self.assertIn("if (position > 64511)", self.main)


if __name__ == "__main__":
    unittest.main()
