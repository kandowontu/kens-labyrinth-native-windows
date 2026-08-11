# Native cheat menu

Press **Ctrl+Alt+F1 while actively playing** to open the menu. Use Up/Down to
navigate, Left/Right to adjust the selected chapter or level, Enter or Space
to select, and Escape to return to the game. The menu pauses the game clock
while it is open and remembers the last selected row.

| Entry | Behavior |
| --- | --- |
| Toggle all | Enables every persistent option; selecting it again when all are enabled disables them |
| God mode | Restores full health, cancels a death in progress, and blocks projectiles, monsters, fans, and holes |
| Noclip | Bypasses wall collision while keeping the player inside safe map bounds |
| Money refill | Restores the coin balance to 999 whenever it changes |
| Key refill | Keeps both key types available |
| Max weapons | Keeps all three weapon power levels and lightning at level 6 |
| Powerup refill | Keeps the purple/green potions and both protective cloaks active |
| Life refill | Keeps nine extra lives available |
| Compass | Keeps the compass/map aid available |
| Restore full health | One-shot heal and recovery from a death animation |
| Next board | Uses the original next-level transition and board loader |
| Chapter | Selects any chapter present in the installed game; Left/Right adjusts it and Enter cycles forward |
| Level | Selects any level in that chapter; Left/Right adjusts it and Enter cycles forward |
| Warp to Chapter/Level | Immediately reloads the selected board through the normal board and music loader; selecting the current board restarts it |
| Return to game | Closes the menu |

Refill toggles stop replenishing when disabled but do not delete inventory the
player already has. Noclip and god mode stop immediately. Disabling Compass
explicitly hides it.

Enabling any cheat sets the original `cheated` state, excluding the run from
the high-score table and retaining the game's normal anti-cheat progression
behavior. Cheat toggles are session settings: they persist through loads and
new games until disabled or the program exits, but they are not written into
save files or `settings.ini`.
