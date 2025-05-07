# Changelog - Air Hockey Score System

## Version 1.5.8 – (connbonn)
**Release Date:** May 7, 2025

### Changes & Enhancements:
- Lock-in logic: Points freeze at 7 and require "Next Game" to continue.
- Skinny Scoreboard enhancements:
  - Added fixed height of **150px** for consistent OBS streaming layout.
  - Timeout display now shown below the timer in yellow.
  - Match summary now displays in compact format with player initials (e.g. `G1 A7-5B`).
- Improved keyboard support:
  - `ArrowLeft` / `ArrowRight` to score points.
  - `A` / `D` to trigger timeouts for Player 1 and Player 2.
- Added persistent localStorage for player names and scoreboard lock setting.
- Export options:
  - Match summary as `.txt`
  - Tournament-ready `.csv` export with prompts for location, referee, and notes.
  - Web Share API and clipboard fallback for mobile sharing.
- UI/UX polish:
  - Confetti celebration on match win.
  - Compact view fades in cleanly with `.fade-out` transitions.
  - Streaming scoreboard toggle hides "Lock to Top" option for cleaner layout.

---
