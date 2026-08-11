from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class FullscreenSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.subs = (ROOT / "src" / "lab3d" / "subs.c").read_text(encoding="utf-8")

    def test_alt_enter_toggles_existing_video_surface(self) -> None:
        self.assertIn("SDL_WM_ToggleFullScreen(surface)", self.subs)
        self.assertIn("event.key.keysym.mod&KMOD_ALT", self.subs)
        self.assertGreaterEqual(self.subs.count("sk == SDLK_RETURN"), 2)

    def test_shortcut_does_not_activate_the_enter_binding(self) -> None:
        self.assertGreaterEqual(self.subs.count("newkeystatus[SDLK_RETURN]=0"), 2)


if __name__ == "__main__":
    unittest.main()
