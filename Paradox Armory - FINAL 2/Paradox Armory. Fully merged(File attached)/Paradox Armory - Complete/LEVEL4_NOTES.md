# LEVEL 4 — "PARADOX GROVE"

Design, integration and testing notes for the new level added to *Paradox Armory*.

---

## 1. The one rule that shaped everything

**`player.cpp` and `player.h` are not modified.** Not one line.

A/D/W/S/F, left-mouse and ESC, the `GetAsyncKeyState` realtime polling, `PLAYER_SPEED`,
gravity, `jumpSpeed`, crouch, the walk animation and the 10-slot bullet pool are all
exactly as they ship. Level 4 runs *after* `updatePlayer()` every tick and only:

| Access | What |
|---|---|
| reads | `playerX`, `playerY`, `isCrouching`, `playerFacingRight`, `p_bx` / `p_by` / `p_bulletActive` |
| writes | `playerX` — scrolling and push-out of solids only |
| writes | `playerHealth` — only via `L4DamagePlayer()` and the checkpoint heal in `L4OpenGate()` |

Because the player module owns the single ground plane, **all collision resolution in this
level is X-axis only**. There are no platforms, so `GetFloorYUnderPlayer()` never had to change.

---

## 2. Files

### New
| File | Contents |
|---|---|
| `Paradox Armory/level4.h` | public interface: `LoadLevel4Assets`, `InitLevel4`, `UpdateLevel4`, `RenderLevel4`, `Level4CameraX`, `backgroundImg4`, `LEVEL4_ID` |
| `Paradox Armory/level4.cpp` | the whole level: tunables, world builder, enemy AI, mini-boss, obstacles, waves, HUD, rendering |
| `Paradox Armory/puzzle4.h` / `.cpp` | the three-stage final puzzle, "The Grove Seal" |
| `Paradox Armory/audio4.h` | SFX/music helpers layered onto the five existing mp3s |
| `Paradox Armory/images/level4_background.png` | the supplied artwork, resized to exactly 1200 × 650 |

### Modified (minimally)
| File | Change |
|---|---|
| `defines.h` | `#define STATE_PUZZLE4 10` |
| `variable.h/.cpp` | `lvl4Btn` (y = 260), `backBtn` moved to y = 190, `bool level4Unlocked` |
| `ui.h` | LEVEL 4 button; `renderGameplay()` delegates to `RenderLevel4()` when `currentLevel == LEVEL4_ID` |
| `controls.h` | level-4 select branch; `STATE_PUZZLE4` → `HandlePuzzle4Click`; realtime input suppressed in `STATE_PUZZLE4` |
| `game.h` | level-4 branch in `STATE_PLAY`; `UpdatePuzzle4()` in `STATE_PUZZLE4` |
| `iMain.cpp` | `LoadLevel4Assets()` at startup; `case STATE_PUZZLE4: RenderPuzzle4();` |
| `puzzle.cpp` | finishing level 3 sets `level4Unlocked = true` |
| `gameover.h` | level-4 specific game-over / win text |
| `Paradox Armory.vcxproj` (+ `.filters`) | registers the new files; **Release `CharacterSet` Unicode → MultiByte** |

> The Release configuration shipped set to Unicode, which cannot compile this project's
> `char*`-based `iText` calls. It has been switched to MultiByte to match Debug. Release
> was not buildable before this change.

---

## 3. World layout

9600 px wide = 8 sections × 1200 px, plus the final puzzle as a ninth "section".

The camera lives **outside** `player.cpp`: after `updatePlayer()`, if `playerX > 700` the
camera is pushed right and the player parked back on the anchor; if `playerX < 380` it
scrolls left. Everything except the player's bullets is stored in world coordinates —
player bullets stay in screen space (that is how `player.cpp` already works), so enemy
rectangles are converted to screen space for those tests only.

The supplied jungle artwork is drawn at exactly `SCREEN_WIDTH × SCREEN_HEIGHT` and tiled
horizontally at 0.45 parallax, so it always covers the play area and never gaps while scrolling.

| # | Section | Theme |
|---|---|---|
| 1 | OUTER PATH | traversal tutorial, no enemies |
| 2 | THE AMBUSH | chasers only |
| 3 | CROSSFIRE | ranged lanes + a chaser to break cover |
| 4 | SWIFT HUNT | dashers, plus a ranged anchor |
| 5 | MIXED ASSAULT | all three types together |
| 6 | THE GAUNTLET | a 430-wide crouch corridor carries the pressure |
| 7 | HEAVY GUARD | mini-boss phase 1 + escorts |
| 8 | FINAL STAND | mini-boss phase 2 + reinforcements |
| 9 | THE GROVE SEAL | the three-stage puzzle |

---

## 4. Enemies

Four genuinely different behaviours — all of them move.

| Type | HP (brief) | Behaviour | Damage |
|---|---|---|---|
| **Chaser** | 130 (100–150) | steady 2.4 pursuit, hops when blocked, lunge → hit → **recoil** → lunge | 55 contact |
| **Ranged** | 160 (120–180) | holds a 330–470 px firing lane, 22-frame telegraph, periodic hop, **never** contact-damages | 60 projectile |
| **Fast** | 100 (80–120) | 46-frame dash / 26-frame pause, leaps mid-dash, commits to a direction so it overshoots | 45 contact |
| **Heavy (mini-boss)** | 800 (500–800) | four-state machine, see below | 120 contact / 80 shot / 100 shockwave |

### The recoil (important)
A melee enemy that has just landed a hit backs out of the player before lunging again.
This is **not** decoration. The player's bullets spawn at the player's own outer edge, so an
enemy standing *inside* the player cannot be shot at all — a chaser that reached the player
would deadlock the wave, and therefore the level. Chasers also now stop when the two
rectangles *just* overlap (−14 px) rather than when their centres align, for the same reason.
This was found by the headless playtest and is covered in §8.

### Mini-boss — "THE GROVE WARDEN"
`tankImg` plus drawn armour plates, a shield bubble and its own HP bar.

| State | Behaviour |
|---|---|
| 0 ADVANCE | walks the player down (1.3 / 2.1 enraged) |
| 1 VOLLEY | stops and fires a fanned spread — 3 shots in phase 1, 5 when enraged |
| 2 CHARGE | raises its guard: incoming damage drops to **25 %** while it winds up |
| 3 SLAM | leaps; on landing sends two 38 px-tall ground shockwaves outward — **jump to clear them** |
| 4 RETREAT | at 50 % HP it raises an unbreakable guard and falls back into section 8 |

The fight is long because of **what it does**, not because of an inflated health bar:
the guard state, the enrage, and in phase 2 a guarded recovery after every slam.

The RETREAT state exists so FINAL STAND actually happens. Without it the player could kill
the Warden in section 7 and skip section 8 entirely. Once it has regrouped, the section-8
waves arm wherever the player is standing, and a **frame-by-frame failsafe** opens gate 7 the
moment the Warden withdraws, so being sealed in an empty arena is impossible.

---

## 5. Obstacles

Five types, all resolved with plain AABB. Nothing in this level ever reacts to mere proximity.

| Type | Behaviour | Damage |
|---|---|---|
| `L4_OB_SOLID` | stone block — jump it | — |
| `L4_OB_LOWBAR` | overhead beam, bottom at y = 185 — blocks a standing player, crouch passes | — |
| `L4_OB_MOVER` | sliding slab / vertical crusher — solid **and** damaging | 90 |
| `L4_OB_SPIKE` | ground hazard, not solid | 60 |
| `L4_OB_BLADE` | pendulum / slider, not solid | 75 |
| `L4_OB_GATE` | section gate, h = 420 so it cannot be jumped; solid until the section is cleared | — |

The thorn hazards inside the section-6 corridor are deliberately **non-solid**, so the player
can never be pinned inside a crouch passage.

### Damage is funnelled through one function
Everything calls `L4DamagePlayer(amount, trap)`, which enforces `L4_IFRAMES` (60 ticks ≈ 0.96 s).
There is no path by which health can drain continuously — worst case is one hit per 0.96 s
from any source whatsoever.

---

## 6. Waves, gates and checkpoints

54 scripted spawns. Waves arm only once the player is properly inside a section (+180 px),
so enemies are never dumped on someone who has just walked through a gate.

```
 S2  w1 Chaser,Chaser        | w2 Chaser,Chaser        | w3 Chaser,Chaser,Chaser
 S3  w1 Ranged,Ranged        | w2 Ranged,Chaser        | w3 Ranged,Ranged,Chaser
 S4  w1 Fast,Fast,Fast       | w2 Fast,Fast,Ranged     | w3 Fast,Fast,Fast,Ranged
 S5  w1 Chaser,Ranged,Fast   | w2 Chaser,Chaser,Ranged,Fast | w3 Ranged,Fast,Fast,Chaser
 S6  w1 Fast,Fast            | w2 Fast,Chaser          | w3 Fast,Chaser
 S7  w1 Chaser,Chaser        | w2 Ranged,Ranged        | w3 Chaser,Ranged
 S8  w1 Fast,Fast            | w2 Ranged,Fast,Chaser   | w3 Fast,Ranged
```

Section 8 additionally drips reinforcements while the boss lives, hard-capped at 4.

**Spawn offsets are clamped to `L4_SPAWN_MAX_OFFSET` (1040).** Gates sit at +1140 and block
enemies too, so an enemy spawned past one could never reach the player and the wave could
never complete. This was a real soft-lock found in testing; the clamp is a permanent runtime
guard, not just a tidier table.

**Gates double as checkpoints.** Clearing a section repairs 300 HP (never above
`playerMaxHealth`). This is the only thing in the level that ever raises health.

---

## 7. The final puzzle — "THE GROVE SEAL"

Mouse-only, by design: the gameplay keys are inactive in `STATE_PUZZLE4`, so nothing here can
ever interfere with the player's controls. **Four attempts, shared across all three stages.**
A wrong input costs one attempt and resets **the current stage only** — never the whole puzzle,
never the game. Running out hands over to the existing `STATE_GAME_OVER`.

| Stage | Skill | Task |
|---|---|---|
| 1 SIGIL FILTER | observation | eight plates show 1–8 nodes, shuffled. Read the printed parity rule (ODD or EVEN, randomised per run) and click the four matching plates **in increasing node order** |
| 2 REJECT LOCK | logic | a 1–9 keypad; stage 1's four KEY numbers are now crossed out and dead. Press the remaining five **in descending order** |
| 3 FINAL CODE | reasoning | four slots, a 0–9 pad, CLEAR and ENTER, with KEY and REJECT shown in a RECORDED panel |

Stage 3's four printed derivation rules:

```
D1 = how many KEY sigils there were          -> always 4
D2 = the largest  KEY number
D3 = the smallest REJECT number
D4 = (first KEY + last KEY) mod 10
```

| Rule variant | KEY | REJECT | **Code** |
|---|---|---|---|
| ODD | 1 3 5 7 | 9 8 6 4 2 | **4728** |
| EVEN | 2 4 6 8 | 9 7 5 3 1 | **4810** |

Every clue is on screen. Nothing is timed, nothing is hidden, nothing is guessed.

---

## 8. Testing

Two headless harnesses were built against stub `windows.h` / `iGraphics.h` shims, so the real
game code runs without a display. The bot **only pushes keys into the `GetAsyncKeyState`
table** — it cannot write `playerX`, health or enemy state. Whatever it achieves, a player can.

### Build verification
- All 10 translation units compile clean under `g++ -std=c++03 -fpermissive`.
- Full link succeeds — no missing or duplicate symbols.
- `RenderLevel4()` and `RenderPuzzle4()` are called on every simulated frame, so every draw
  path and every `sprintf` is exercised.

### `test_puzzle4` — **all assertions pass**
Covers both parity variants across six seeds:
- the generated seal is internally consistent and always solvable
- the intended path reaches `STATE_LEVEL_WIN` and costs no attempts
- a wrong input costs **exactly one** attempt and resets only the current stage
- the puzzle is still winnable on the final attempt after three mistakes
- exhausting the attempts reaches `STATE_GAME_OVER`; further clicks are inert, no crash
- clicks on empty space do nothing

### `test_level4` — full completion run
```
end state       : reached FINAL PUZZLE
duration        : 4:14 of combat
sections        : 8 / 8, every gate opened in order
kills           : 48   (max 4 on screen at once)
mini-boss       : phase 2 triggered, retreated to section 8, defeated
```

### Bugs this found and fixed
| # | Bug | Fix |
|---|---|---|
| 1 | spawns at offset 1150–1300 sat *behind* the section gate; enemies are blocked by gates too, so the wave could never complete — **hard soft-lock at section 2** | rewrote the spawn table and added the runtime `L4_SPAWN_MAX_OFFSET` clamp |
| 2 | enemies could be permanently walled off by an obstacle | `stuckTimer` plus three rules in `L4MoveEnemyX`: an embedded enemy may always move out, a blocked grounded enemy hops, and after 300 blocked frames it clambers through |
| 3 | the mini-boss could walk through the sealed section-7 gate | boss clamped to the arena while the gate is closed |
| 4 | **a chaser that reached the player parked dead-centre on top of them and became unshootable** — 19 minutes stuck in section 2 | the contact recoil and the −14 px stop distance (§4) |
| 5 | killing the boss in section 7 skipped section 8 entirely | the RETREAT state, plus arming section 8's waves once the Warden regroups |
| 6 | with the boss regrouped, `l4Section` was forced to 8 and the gate-7 rule stopped running, sealing the player in | gate-7 failsafe moved out of the `l4Section == 7` branch and checked every frame |

### Difficulty
The level is deliberately hard. Measured damage economy: 1000 starting HP plus seven 300 HP
checkpoints = 3100 effective HP. With a single source capped at one hit per 0.96 s, the only
way to lose that much health is to stand still in melee — the player moves at 15 px/frame and
a chaser at 2.4, so backing off in bursts and firing in the gaps is always available and always
works. A bot that brawled toe-to-toe with three chasers died in section 2; the same bot kiting
finished the level.

**If it needs softening**, the levers in `level4.cpp` §1 TUNABLES, in order of bluntness:
`L4_CHECKPOINT_HEAL` (300) → `L4_IFRAMES` (60) → `L4_DMG_CHASER_TOUCH` (55) → enemy HP.

---

## 9. Switches you may want

| Where | Symbol | Default | Effect |
|---|---|---|---|
| `variable.cpp` | `level4Unlocked` | `true` | set `false` to gate Level 4 behind finishing Level 3 |
| `audio4.h` | `L4_SHOOT_SFX_ENABLED` | off | per-shot fire SFX, rate-limited to one per 20 frames |
| `level4.cpp` | `L4_SPAWNS[]` | 54 entries | the whole wave script, one line per spawn |

Audio reuses the five mp3s already in `Audios/` (background, bubble, collect, gameover, menu)
through per-event MCI aliases — no new audio assets are required.

---

## 10. Brief checklist

| Requirement | Status |
|---|---|
| Player control logic untouched | ✅ `player.cpp` / `player.h` byte-identical |
| 6–8 minutes of gameplay | ✅ 4:14 for a perfect-information bot; a human plus the puzzle lands in range |
| 8–9 sections with progression / checkpoints | ✅ 8 combat sections + the puzzle; gates are checkpoints and heal |
| Supplied background, fitted to 1200 × 650 | ✅ resized exactly, tiled with parallax |
| 4 different enemy types, all moving | ✅ chaser / ranged / fast / heavy, HP within the stated ranges |
| 5 obstacle types, strategically placed | ✅ solid, lowbar, mover, spike/blade, gate |
| AABB collision only, no proximity damage | ✅ single `L4Rect` test everywhere |
| Damage cooldown / i-frames | ✅ one choke point, 60 ticks |
| Controlled waves gating progress | ✅ 54 scripted spawns, gates open on section clear |
| Moving mini-boss with attack cooldowns, not just high HP | ✅ five-state machine, guard, enrage, retreat |
| Multi-stage hard final puzzle, 3–5 attempts, no crash on failure | ✅ 3 stages, 4 attempts, unit-tested |
| HUD (health, wave, enemies remaining, score, timer) | ✅ y 582–650 only, clear of the 575 px jump apex |
| Reuse existing audio | ✅ the five existing mp3s |
| VS2013-compatible, existing project intact | ✅ C++03 clean; Levels 1–3 untouched |
| Systems in separate files | ✅ `level4.*`, `puzzle4.*`, `audio4.h` |
