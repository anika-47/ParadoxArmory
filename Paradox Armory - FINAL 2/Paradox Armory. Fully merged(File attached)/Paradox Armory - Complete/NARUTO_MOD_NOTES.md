# Naruto player mod

Changes on top of the original Level 4 project (everything else is untouched):

- `Paradox Armory/player.cpp`
  - `playerHealth` / `playerMaxHealth` defaults 10000 -> 20000.
  - Naruto state + `LoadNarutoAssets()` (loads `images\naruto_*.png`).
  - `SetPlayerCharacter(4)` selects Naruto (same gun/bullet values Level 4 used before).
  - `FirePlayerBullet()` starts a visual-only shooting timer.
  - `UpdateNarutoAnimation()` (idle timer / shoot timer), called from `updatePlayer()`.
  - `DrawNarutoPlayer()` draws into the same box as the old sprite; `RenderPlayer()` calls it when Naruto is active.
- `Paradox Armory/level4.cpp`
  - `L4_PLAYER_MAX_HP` 1000 -> 20000.
  - `InitLevel4()` calls `SetPlayerCharacter(LEVEL4_ID)` instead of `SetPlayerCharacter(2)`.
  - HUD text "LEVEL 4 : PARADOX GROVE" -> "LEVEL 4".
- `Paradox Armory/images/naruto_*.png` (27 new files): idle x4, walk x7, shoot, crouch (each `_r` / `_l`), plus `naruto_flash.png`.

To use Naruto in Levels 1-3 as well, call `SetPlayerCharacter(4)` from those levels' init.
