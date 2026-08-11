from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ModalScreenPresentationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.opengl = (ROOT / "src" / "lab3d" / "opengl.c").read_text(encoding="utf-8")

    def function_body(self, signature: str, next_signature: str) -> str:
        body = self.opengl[self.opengl.index(signature) :]
        return body[: body.index(next_signature)]

    def test_front_updates_are_mirrored_to_the_back_buffer(self) -> None:
        body = self.function_body("void openGLDrawFront", "void openGLDrawBack")
        self.assertIn("frontbufferdrawing=1", body)
        self.assertIn("glDrawBuffer(GL_FRONT_AND_BACK)", body)
        self.assertNotIn("glDrawBuffer(GL_FRONT);", body)

    def test_front_flushes_are_presented_through_a_swap(self) -> None:
        body = self.function_body("void openGLFlush", "/* Fade")
        self.assertIn("if (frontbufferdrawing)", body)
        self.assertIn("SDL_GL_SwapBuffers();", body)
        self.assertIn("glFlush();", body)

    def test_normal_game_rendering_still_targets_only_the_back_buffer(self) -> None:
        body = self.function_body("void openGLDrawBack", "void openGLFlush")
        self.assertIn("frontbufferdrawing=0", body)
        self.assertIn("glDrawBuffer(GL_BACK)", body)


if __name__ == "__main__":
    unittest.main()
