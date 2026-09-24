#ifndef PUZZLE4_H
#define PUZZLE4_H

// =====================================================================
// LEVEL 4 FINAL PUZZLE - "THE NINE-TAILS SEAL"
// =====================================================================
//
// A five-stage lock. It opens only when the player steps onto the seal
// altar at the end of the level (see level4.cpp, "the seal approach").
// It is mouse-only on purpose: the gameplay controls (A/D/W/S/F) are
// inactive in STATE_PUZZLE4, and nothing here adds a key binding that
// could ever interfere with them.
//
// Everything is built on ONE rule that is printed on screen at all
// times: the chakra-nature cycle
//
//        FIRE > WIND > LIGHTNING > EARTH > WATER > FIRE      (A > B : A overpowers B)
//
//   STAGE 1  THE WITNESSES   observation + deduction - five plates, three
//                            testimonies, exactly ONE of which lies. Find
//                            the WARDEN plate.
//   STAGE 2  THE CHAIN       pattern - seal the plates in the order the
//                            Warden's chakra consumes them, skipping the
//                            drained plate (which one is skipped is read
//                            off the plates, not told).
//   STAGE 3  ECHO            memory + transformation - six natures flash;
//                            answer each with the nature ONE STEP along
//                            (or against) the cycle. Which direction depends
//                            on the plate that ended the chain in stage 2.
//   STAGE 4  LOGIC LOCK      combination - three dials, several rules that
//                            refer to results of stages 1-3. Exactly one
//                            dial setting satisfies all of them.
//   STAGE 5  FINAL SEAL      derivation - a four-digit code built from the
//                            results of every earlier stage.
//
// Five attempts, shared across all stages. A wrong input costs one
// attempt and resets the CURRENT stage only (never the whole puzzle, and
// never the game). Running out of attempts hands over to the existing
// STATE_GAME_OVER screen. Solving it sets puzzleSolved and hands over to
// the existing STATE_LEVEL_WIN screen.
//
// Every clue needed is on screen. Nothing is timed against the player,
// nothing is hidden and nothing is guessed: the layout of the plates, the
// testimonies, the flash sequence, the lock rules and the answer are
// generated per run, but the generator only ever accepts a puzzle whose
// answer is UNIQUE under the printed rules (it checks by enumeration).
// =====================================================================

void InitPuzzle4(void);
void RenderPuzzle4(void);
void UpdatePuzzle4(float dt);
void HandlePuzzle4Click(int mx, int my);

#endif // PUZZLE4_H
