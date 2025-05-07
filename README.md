# Air Hockey Score System

This web app is designed for air hockey tournaments and casual matches. It features a simple, stylish scoreboard with timer, score tracking, match history, and match summary export. It's built to be mobile-friendly and works well in portrait mode for tablets and phones.

## 🎯 Features

- Score tracking for two players
- Timeout system with countdown
- Skinny streaming scoreboard view
- Match timer with pause/resume
- Match summaries (full and compact)
- Export options: CSV and TXT
- Confetti celebration for match winner
- Keyboard hotkeys: arrow keys, A/D for timeouts, Enter to advance game
- Table number banner via URL (e.g. `?table=4`)

## 📋 New in v1.5.9 – (TD)

- 🆕 **Tournament Director Page (TD.html)**:
  - Create a match link prefilled with player names and match length
  - Optional Table # field adds `&table=##` param to the URL
  - Generates a QR Code and a copy-to-clipboard button
  - Collapsible how-to-use guide embedded for staff or TDs
- Main scoreboard now reads and displays a `table` param as a yellow 📍 Table banner

## 💡 Usage

Visit the [main scoreboard](https://petesimple.github.io/airhockey-score-system/) to start a match manually, or use the new [Tournament Director Tool](https://petesimple.github.io/airhockey-score-system/TD.html) to generate links.

## 🛠 Tech Stack

- HTML/CSS/JavaScript
- LocalStorage for name persistence
- QRCode.js for TD QR generation
- Canvas Confetti for celebrations
- Fully offline-capable via Service Worker

## 🧠 Ideas Welcome

Open an issue or start a discussion to suggest new features, like:
- Audio feedback for goals/timeouts
- Bracket or Elo integration
- OBS compatibility enhancements

---

Built with 💙 by [Pete](https://github.com/petesimple) and ChatGPT.
