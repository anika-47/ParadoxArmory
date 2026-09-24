# Final puzzle upgrade - "THE NINE-TAILS SEAL" + two new hazards

This supersedes sections 5 (obstacle table, additions only) and 7 (the old three-stage
"Grove Seal") of `LEVEL4_NOTES.md`. Everything else in the game is unchanged.

## Files changed
| File | Change |
|---|---|
| `Paradox Armory/puzzle4.cpp`, `puzzle4.h` | old 3-stage puzzle replaced by the 5-stage Nine-Tails Seal (same four public functions) |
| `Paradox Armory/level4.cpp` | two new obstacle types + altar ("the seal approach"); puzzle now starts at the altar instead of the instant the Warden dies |
| `Tests (headless)/test_puzzle4.cpp` | rewritten for the new puzzle (not part of the VS project) |

No new asset files. The puzzle screen reuses `naruto_idle_0_r.png` and `naruto_flash.png`;
everything else is drawn with iGraphics shapes. No `.vcxproj` change is needed.

## The two new obstacles (final area only, awake after the Warden falls)
| Type | Behaviour | Damage |
|---|---|---|
| `L4_OB_WISP` | chakra wisp on a **figure-eight** patrol (2-D path, speed swells/eases +-40 %); path drawn as a dotted guide; hit box 6 px inside the drawn orb | 80 |
| `L4_OB_PYLON` | full-height energy barrier on a 240-tick cycle: dormant 110 -> charging 50 (warning, extent outlined) -> **active 60** -> cooling 20 | 120 |

Both are non-solid (cannot pin or block the player), both go through `L4DamagePlayer()` and
its 60-tick invulnerability, both are asleep (not drawn, not updated, no collision) until the
Warden is dead, and get a 100-tick harmless grace period when they wake. Layout, in world x:
pylon 8845 -> wisp 9225 -> pylon 9445 (half a cycle out of step) -> altar 9480.

The puzzle opens when the player's left edge reaches world x 9470.

## The puzzle
One rule is printed on screen the whole time - the nature cycle
`FIRE > WIND > LIGHTNING > EARTH > WATER > FIRE` (A > B: A overpowers B) - with the nature numbers
FIRE 0 ... WATER 4.

1. **Witnesses** - five plates (nature, chakra orbs, position). Three testimonies, exactly one lies. Click the Warden.
2. **Chain** - seal plates in the order the Warden's chakra consumes them (Warden, what it overpowers, ...), skipping the plate with one orb.
3. **Echo** - six natures flash; press each one's neighbour on the cycle: EVEN last-plate chakra -> one step along the arrows, ODD -> one step against. First viewing free, two replays.
4. **Logic lock** - three dials, 4-6 rules referring to the results above; exactly one setting fits.
5. **Final seal** - four digits: Warden's chakra; sum of dial numbers (last digit); number of different natures in the echo answer; (first echo answer + last chain nature) last digit.

Five attempts shared by all stages. A wrong input costs one and resets only the current stage;
running out uses the existing game-over state; success uses the existing level-win state.

## Verification (headless, `Tests (headless)/test_puzzle4.cpp`)
Over 20 000 generated puzzles the test re-reads the printed sentences with an independent parser
and confirms stage 1 has exactly one surviving (plate, liar) pair, stage 4 exactly one of 125 dial
settings, and that stages 2, 3 and 5 match independently derived values; 3 000 of them are solved
through the real click handler to `STATE_LEVEL_WIN`.
