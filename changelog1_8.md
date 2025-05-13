# Air Hockey Score System – Changelog

## ver1.8 – (puckin) – 2025-05-13
### 🛠 New Features & Improvements
- 🧼 Between-Games Timer auto-clears when any player scores a point.
- 🧠 `scorePoint()` now guarantees cleanup of between-game timer if active.
- 🏁 "Start Next Game" button now properly re-labels as "Declare Winner" when match point is reached.
- ⏱️ Google Sheet timestamps now consistently preserve full datetime (no longer stripped in archive).
- 🔁 Enhanced scoreboard logic to ensure tight behavior on game transitions.
- 🧪 Minor styling polish and pulse animations retained for winners.
- 🧪 Confirmed `scorePoint()` and `startNextGame()` match v1.7.3 with improved clarity.

---

## Previous Versions
See earlier changelogs for:
- v1.7.3 – (inbetweenytimer)
- v1.7 – (miticlift)
- v1.6 – (skunk)
- v1.5.8 – (connorbonnor)
