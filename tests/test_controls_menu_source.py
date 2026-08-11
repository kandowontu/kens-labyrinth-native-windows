from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ControlsMenuSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.header = (ROOT / "src" / "lab3d" / "lab3d.h").read_text(encoding="utf-8")
        cls.setup = (ROOT / "src" / "lab3d" / "setup.c").read_text(encoding="utf-8")
        cls.main = (ROOT / "src" / "lab3d" / "lab3d.c").read_text(encoding="utf-8")
        cls.subs = (ROOT / "src" / "lab3d" / "subs.c").read_text(encoding="utf-8")

    def test_main_menu_exposes_controls(self) -> None:
        self.assertIn('strcpy(&textbuf[0],"Controls")', self.subs)
        self.assertIn("mainmenuplace == 4) controlsmenu()", self.subs)
        self.assertIn("getselection(88,47,mainmenuplace,10)", self.subs)

    def test_expanded_menus_keep_selectors_inside_their_frames(self) -> None:
        self.assertIn("drawmenu(192,168,menu)", self.subs)
        self.assertIn("getselection(-12,3,selection,16)", self.setup)

    def test_scaled_hud_has_no_solid_separator_band(self) -> None:
        statusbar = self.subs[self.subs.index("void statusbaralldraw()") :]
        statusbar = statusbar[: statusbar.index("statusbardraw(0,0,32,32")]
        self.assertIn("screenbuffer[i+240*screenbufferwidth]=0xff", statusbar)

    def test_all_keyboard_actions_are_exposed(self) -> None:
        self.assertIn("#define numcontrolkeys 26", self.header)
        self.assertIn("keynames[numcontrolkeys]", self.setup)
        self.assertIn("newdefaultkey[numcontrolkeys]", self.setup)
        self.assertIn("action=page*13+choice", self.setup)

    def test_fixed_system_keys_now_use_bindings(self) -> None:
        actions = (
            "CONTROL_CHEAT_MENU",
            "CONTROL_SCREENSHOT",
            "CONTROL_SOUND_DOWN",
            "CONTROL_SOUND_UP",
            "CONTROL_MUSIC_DOWN",
            "CONTROL_MUSIC_UP",
            "CONTROL_GAMMA_DOWN",
            "CONTROL_GAMMA_UP",
        )
        for action in actions:
            self.assertIn(f"newkeydefs[{action}]", self.main)
        for fixed in ("SDLK_F5", "SDLK_F6", "SDLK_F7", "SDLK_F8", "SDLK_F9", "SDLK_F10"):
            self.assertNotIn(f"newkeystatus[{fixed}]", self.main)

    def test_bindings_persist_with_legacy_compatibility(self) -> None:
        self.assertIn('"extended-controls 1\\n"', self.setup)
        self.assertIn("for(i=0;i<numkeys;i++)", self.setup)
        self.assertIn("for(i=numkeys;i<numcontrolkeys;i++)", self.setup)
        self.assertIn("savesettings();", self.setup)

    def test_any_sdl_keyboard_key_can_be_captured(self) -> None:
        capture = self.setup[self.setup.index("static int capturecontrolkey") :]
        capture = capture[: capture.index("void controlsmenu")]
        self.assertIn("event.type==SDL_KEYDOWN", capture)
        self.assertIn("event.key.keysym.sym<SDLKEYS", capture)
        self.assertNotIn("event.key.keysym.sym==SDLK_ESCAPE", capture)


if __name__ == "__main__":
    unittest.main()
