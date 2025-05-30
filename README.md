# Air Hockey Score System (AHSS)

Welcome to the **Air Hockey Score System**, a fully-featured web app for tracking competitive air hockey matches—used in tournaments, casual games, and stream overlays. 🏒💥

## 🚀 Features

- 🎮 Scorekeeping for best-of-3, 5, or 7 sets
- 🧠 Player name memory (via localStorage)
- ⏱️ Animated match timer with pulse
- ⏸️ Clickable timer for pause/resume
- 🕹️ Arrow key & keyboard control support
- ⏳ 10-second timeout system (one per player)
- 🔁 Flip sides button for fair gameplay
- 📺 "Skinny" scoreboard view for stream overlays
- 📌 Optional "lock-to-top" scoreboard
- 🔔 Confetti animation and sound on match win
- 📋 Compact game summary in initials (e.g., `G1 A7-5B`)
- 🦨 Skunk detection (7-0) with emoji in scoreboard
- 🔘 Winner and loser pips in skinny scoreboard (● and ○)
- 📦 Export match results as:
  - Text summary
  - Clipboard share
  - Tournament-ready CSV file

### 🧑‍⚖️ Tournament Director (TD) Page

The **TD.html** page lets you:
- Generate match links with player names and match length
- Create a scannable QR code for each match
- Copy the match URL to clipboard
- Access a built-in collapsible help guide
- Use as a standalone PWA for tournaments

## 📦 Setup

No installation required—just open `index.html` or `TD.html` in your browser.

To use as a PWA (Progressive Web App):
- Add to Home Screen on iOS or Android
- Works offline with service worker included

## 🧪 Keyboard Shortcuts

| Key        | Action                        |
|------------|-------------------------------|
| ←          | Score for left player         |
| →          | Score for right player        |
| A / D      | Call timeout (left/right)     |
| Enter      | Trigger "Next Game" button    |

## 📜 Version

**v1.6 – (skunk)**  
Now includes skunk pips in the skinny scoreboard 🦨 and polish to match end behavior!

## 📁 Files

- `index.html` – Main app
- `TD.html` – Tournament Director match tool
- `manifest.json` / `manifestTD.json` – PWA manifests
- `service-worker.js` / `service-worker-td.js` – Offline support
- `icon-192.png`, `iconTD-192.png`, `logo.png` – App assets

## ❤️ Credit

Created by [Pete Lippincott](https://github.com/petesimple) with help from ChatGPT.  
Built for the air hockey community. Feedback welcome!

---

🏓 May the best mallet win.
