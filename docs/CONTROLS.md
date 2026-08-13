# Remappable controls

Choose **Controls** from the main menu. The two-page controls screen exposes
all 26 keyboard actions. Select an action with Up/Down and Enter, then press
the key that should activate it. Escape is a valid binding while the capture
prompt is visible; use Escape from the action list to return to the main menu.

Bindings are written to `settings.ini` immediately. Existing configuration
files remain compatible: the original 18 bindings keep their positions and
the eight new system bindings are stored in a versioned extension at the end
of the file. Duplicate bindings are allowed. **Reset all controls to defaults**
restores every action on both pages.

## Page 1: Movement and actions

| Action | Default |
| --- | --- |
| Move forward / backward | Up / Down |
| Turn left / right | Left / Right |
| Strafe modifier | Right Ctrl |
| Strafe left / right | Q / E |
| Stand high / low | A / Z |
| Run | Left Shift |
| Fire | Left Ctrl |
| Red fireballs / green bouncy bullets / heat-seeking missiles | F1 / F2 / F3 |
| Unlock, open, close, or use | Space |

The lightning-bolt pickup shown in the fourth HUD box is a passive range
upgrade, not a selectable fourth weapon. Each lightning level keeps every
projectile alive longer, up to level 6. It takes effect automatically, so use
the normal Fire control with any of the three weapons.

## Page 2: System controls

| Action | Default |
| --- | --- |
| Original extra-life cheat | Backspace |
| Raise/lower status bar | Enter |
| Pause | P |
| Mute | M |
| Main menu | Escape |
| Cheat menu | F4; Ctrl+Alt+F1 remains as a fallback |
| Screenshot | F12 |
| Sound volume down/up | F5 / F6 |
| Music volume down/up | F7 / F8 |
| Gamma down/up | F9 / F10 |

**Alt+Enter** toggles fullscreen mode globally. It is reserved as a window
shortcut rather than a remappable gameplay action.

The Controls screen includes **Mouse mode: LOOK/MOVE**. Look uses horizontal
mouse motion to turn; Move uses it to strafe. Vertical mouse motion moves
forward/back in both modes.

Mouse fire and use remain available alongside their remappable keyboard
bindings when mouse input is enabled.
