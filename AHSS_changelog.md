# Air Hockey Score System – Changelog

## v1.6 – (skunk) – 2025-05-08
- 🦨 Added skunk detection: if a player wins a game 7–0, the loser gets a skunk emoji instead of a pip in the skinny scoreboard.
- ●○ Updated pip system in skinny scoreboard:
  - Filled circle (●) for a win
  - Empty circle (○) for a loss
  - Skunk emoji for a shutout loss
- Dynamic pip line updates for both players after each game.
- Retained pulse animation on winner after match ends.
- Version label updated to reflect "ver1.6 – (skunk)"

## v1.5.9 – (WWCD)
- Match button now changes to "Declare Winner" on final game instead of "Start Next Game".
- Added keyboard support for:
  - Left Arrow: score left player
  - Right Arrow: score right player
  - A/D keys: call timeout for each player
  - Enter: trigger Next Game button if visible
- Match summary now includes compact notation (e.g., `G1 A7-3B`)
- Skinny scoreboard gets:
  - Locked fixed-height styling for better streaming layouts
  - Real-time updates for timer, timeout countdowns, and game status
  - Option to toggle "Streaming Scoreboard" and lock/unlock top bar

## v1.5.8 – (connbonn)
- Game score now caps at 7 points
- "Next Game" button only increments game count when pressed
- Fixed skinny scoreboard layout height to 150px
- Initials shown in compact match summary format
- Match summary appears below skinny scoreboard after final game

## v1.5.6 – (streaming)
- Introduced "Skinny Scoreboard" mode for streaming
- Fade-in/fade-out transitions for UI elements
- Timeout countdown display added (10 seconds)
- Export options added: match summary (.txt), share via navigator.share or clipboard, and full .CSV
- Scoreboard lock-to-top feature added
- Fixes for scoreboard layout and player name persistence
- Confetti celebration on match win 🥳
