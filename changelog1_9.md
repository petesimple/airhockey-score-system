# 📋 CHANGELOG — AHSS & Tournament Director Companion

## v1.9.0 (Tournament Director) — "Page Master"
**Released: May 22, 2025**

### ✨ New Features
- **Pagination in Match Log**: Automatically splits the log into pages of 25 matches each — perfect for 300+ match days.
- **Search Functionality**: Instantly filter the match log by player name, table number, or referee.
- **Page Navigation Controls**: "Next" and "Prev" buttons make it easy to scroll through massive logs.

### 🧑‍🏫 How-To Guide Expanded
- Added detailed instructions on using search, pagination, export, clear, and logo upload functions.

---

## v1.8.8 (AHSS) — "FIA"
**Released: May 21, 2025**

### 🛠️ Game Summary Fixes
- Compact and full summaries now correctly reflect scores and winner initials across side swaps.
- Sudden death outcomes now correctly log in the summary with accurate pips and initials.

### 🔧 Logic Refinement
- End-of-game state capture improved: `winnerScore`, `loserScore`, `winnerInitial`, `loserInitial`, and `suddenDeathLoser` now preserved prior to `swapSides()`.

---

## v1.8.1 (AHSS) — "PuckinPolish"
**Released: May 14, 2025**

### 💄 Polish & UX Fixes
- Additional spacing and alignment improvements on scoreboard and referee banner.
- Skinny scoreboard height now locked for consistent OBS layout.

---

## v1.6.0 (AHSS) — "Skunk" Series
**Released: May 12, 2025**

### 📊 Google Sheets Integration
- Match data now posts directly to the `AHSS` Google Sheet via Apps Script (no popups).
- Includes full player info, scores, match metadata, and skunk flag.

---

## v1.5.6 (AHSS) — "Streaming Mode"
**Released: May 6, 2025**

### 🎥 Skinny Scoreboard Mode
- Toggleable slimline scoreboard ideal for tournament stream overlays.
- Auto-generates compact match summary (e.g., `G1 A7-5B`) on match end.

---

## TD Companion Initial Launch — v1.0.0
**Released: May 7, 2025**

### 🚀 Core Features
- Match link generator with player names, table, ref, and match length.
- Instant QR code for scanning.
- Match log with time/date.
- Export to CSV and localStorage support.
- Custom logo upload via long-press.

---

Let us know if you'd like to fork any features into a Ref-only view or Broadcast mode!
