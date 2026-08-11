# Credits and acknowledgements

This project exists because its original creators and later porting projects
made their work and history available. The names below are retained from the
original game documentation and the LAB3D/SDL documentation.

## Ken's Labyrinth

- **Ken Silverman** — game design, programming, music, artwork, sound effects,
  and AdLib emulation.
- **Andrew Cotter** — level design/board maps, artwork, sound effects, and the
  original hints guide.
- **Mikko Iho / Future Crew** — artwork. The 1993 manual credits Future Crew;
  the later LAB3D/SDL credits identify Mikko Iho.
- **Epic MegaGames** — publisher of the 1993 v2.x release and party to the
  later freeware release.
- **Advanced Systems** — publisher of the original v1.x releases.

The original game's credits are preserved in `game-data/v2.1/helpme.doc`.
Ken Silverman's official page provides the full registered v2.1 game as
freeware and the corresponding source release:
<https://advsys.net/ken/klab.htm>.

## LAB3D/SDL

- **Jan Lonnberg** — LAB3D/SDL port and its SDL/OpenGL, input, sound, MIDI,
  endian, platform, and renderer work.
- **Ken Silverman** and **Danny Desse** — LAB3D/SDL testing.

The complete upstream history and original credit block are preserved in
`src/lab3d/readme.txt`.

## Native Windows release and analysis

- **kandowontu** — project maintenance, Windows release, preservation,
  reverse-engineering workflow, extracted-asset catalog, menus, controls, and
  native-port integration in this repository.
- **KKIT/SDL and KKIT/CLI by kaimitai** — archive-format extraction provenance
  used by the asset-ripping workflow: <https://github.com/kaimitai/klabkit-sdl>.

## Runtime libraries

- **Simple DirectMedia Layer**, created by **Sam Lantinga** and maintained by
  the SDL contributors: <https://www.libsdl.org/>.
- **sdl12-compat** contributors — SDL 1.2 compatibility implemented on modern
  SDL. Its zlib license is preserved in `third_party/sdl12-compat/LICENSE.txt`.
- **dr_mp3 by David Reid (mackron)** — incorporated by the bundled SDL runtime
  under public-domain or MIT-0 terms, as noted by the SDL license.

No affiliation with or endorsement by Ken Silverman, Epic Games, Future Crew,
the SDL project, or any other credited party is implied.

