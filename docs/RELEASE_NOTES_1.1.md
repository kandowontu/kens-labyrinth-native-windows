# Ken's Labyrinth Native Windows Port 1.1

This release improves gameplay input and control customization:

- Mouse-look supports continuous 360-degree turning while the cursor remains confined to the game window.
- Mouse confinement is released automatically for menus and when the game loses focus.
- Added a Controls-screen toggle between mouse Look and mouse Move modes.
- Added dedicated remappable strafe-left and strafe-right actions.
- Rebinding a key now removes its previous binding from any other action.
- Fixed the duplicate mouse-mode label in the system-controls page.

The release remains self-contained: the Windows ZIP includes the executable,
bundled SDL runtime, and required game data. No Visual C++ Redistributable or
other separately installed runtime is required.
