# 🏒 Air Hockey Score System

A web-based scoreboard app designed for live air hockey match tracking. Built for tournaments, livestreams, and casual games alike — fast, responsive, and packed with features for referees and players.

---

## 🚀 Features

- 🎮 **Live Score Tracking** – Tap to add/remove points for each player.
- 🧑‍🤝‍🧑 **Player Name Input** – Customizable names, with persistent UI layout.
- ⏱️ **Match Timer** – Click to start/pause with animated pulse indicator.
- ⌛ **Timeout Clock** – 10-second countdown for per-player timeouts.
- 🔒 **Lock Scoreboard to Top** – Great for OBS streaming or mobile view.
- 🔁 **Game History** – Tracks each game’s final score.
- 🏆 **Match Winner Detection** – Auto-confetti and summary export.
- 📄 **Summary & CSV Export** – Save or share match results with one click.
- 📱 **Mobile Friendly + PWA Support** – Works offline after first load!

---

## 🛠️ Installation

### Run It Live
Access the app directly at:
[https://petesimple.github.io/airhockey-score-system/](https://petesimple.github.io/airhockey-score-system/)

### Local Setup
Clone this repo and open the `index.html` file in your browser:
```bash
git clone https://github.com/petesimple/airhockey-score-system.git
cd airhockey-score-system
open index.html
```

---

## 🎯 Usage

1. Enter player names and match length.
2. Click “Start Match” — the scoreboard appears.
3. Tap player buttons to score points or call timeouts.
4. Use the **Lock to Top** option for streaming or fixed-view display.
5. Tap the timer to pause/resume.
6. Export the match summary or CSV once the match concludes.

---

## 💡 Development Notes

- Built with vanilla HTML, CSS, and JavaScript.
- Uses `localStorage` to persist scoreboard lock state.
- PWA-ready with offline caching (via `service-worker.js`).
- Designed for fast input and clean mobile responsiveness.

---

## 📦 Changelog

See [CHANGELOG_v1.4.5.md](CHANGELOG_v1.4.5.md) for the latest updates.

---

## 🤝 Contributing

Suggestions, bug reports, and pull requests are welcome! Please:
- Fork the repo
- Create a branch
- Make your changes
- Submit a PR

---

## 🧠 Inspiration

Created by Pete Lippincott — blending code, music, and competitive air hockey since day one.

---

## 📜 License

[MIT License](LICENSE)
