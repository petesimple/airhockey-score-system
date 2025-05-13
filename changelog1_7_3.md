## [1.7.3] – 2025-05-13
### 🧠 Enhancements
- Added logic to **suppress the "Between Games" timer** if the match is already won (either by point logic or via confetti trigger).
- Ensures no awkward "Next Game" countdown appears when there's literally no next game to be played. 😅
- Refined `scorePoint()` logic to pre-check if the **final game is about to be won** and cancel in-between countdown accordingly.

### ✅ Behavior Fixes
- `clearBetweenGamesTimer()` is now more strictly enforced at match end.
- Ensures match-end UI is clean and doesn't leave lingering timers post-confetti.
- Handles edge case where players hit 7 in final game but haven't clicked "Next Game" yet.

### 🪄 UI / UX Polish
- Match-end scenario now feels tighter and more logical, especially during livestreams or recaps.
- No timer countdown distracting viewers when the match is already over.

---

## [1.7.2] – 2025-05-13
### 💡 New Features
- Introduced a **2-minute countdown timer** between games!
  - First minute is **yellow**
  - Final minute is **red**
  - Timer is dismissible by tapping/clicking it.
- Timer lives under the main scoreboard and also syncs to the Skinny Scoreboard view.

---

## [1.7.1] – 2025-05-12
### 🪛 Bug Fixes & Small Touches
- Fine-tuned behavior of between games countdown when entering final game or match point scenarios.
