Air Hockey Score System (AHSS)

Welcome to the Air Hockey Score System (AHSS) - a powerful, flexible web app for running competitive air hockey matches at tournaments, weekly clubs, casual play, and live streams. 🏒

AHSS is designed to be fast, reliable, offline-capable, and customizable, with a clean separation between match logic, data, and visual presentation.

⸻

🚀 Core Features

🎮 Match & Scorekeeping
	•	Supports best-of 3, 5, or 7 games
	•	First to 7 goals wins a game
	•	Sudden death support
	•	Flip sides at any point for fair play
	•	Undo scoring support
	•	Visual winner / loser indicators

🧠 Player & Match Memory
	•	Player names persist via localStorage
	•	Match state survives accidental refreshes
	•	Clean reset between matches

⏱️ Timer & Timeout System
	•	Match timer with visual pulse
	•	Tap/click to pause or resume
	•	One 10-second timeout per player
	•	Timeout countdown displayed independently
	•	Timeout cancellation supported

⸻

📺 Streaming & Display Tools

Skinny Scoreboard (Stream View)
	•	Compact overlay-friendly scoreboard
	•	Winner and loser pips (● / ○)
	•	Skunk detection (7-0) with visual marker 🦨
	•	Compact match summary (e.g. G1 A7-5B)
	•	Optional mirrored view for opposite-side refs
	•	Lock-to-top option for clean OBS framing

Broadcast-Ready Design
	•	Optimized for OBS and screen capture
	•	Stable layout that survives side swaps
	•	Minimal ref UI for stream-only use

⸻

🎨 Custom CSS Tool (v2.8.9)

AHSS now includes a Custom CSS Tool, allowing full visual customization without touching core logic.

With Custom CSS you can:
	•	Change fonts, colors, and spacing
	•	Resize and reposition names and scores
	•	Customize the stream view independently
	•	Apply league or event branding
	•	Create reusable “skins” for broadcasts

Custom styles override defaults safely and are fully optional.

📘 Documentation:
https://petesimple.github.io/airhockey-score-system/documentation.html

⸻

⌨️ Input & Controls

Keyboard & Controller Support
	•	Keyboard scoring and match control
	•	Customizable hotkey mappings
	•	Controller-friendly input handling
	•	Designed for referee tablets and kiosks

Key	Action
←	Score left player
→	Score right player
A / D	Timeout (left / right)
Enter	Next Game


⸻

📋 Match Data & Export
	•	Automatic match summaries
	•	Export results as:
	•	Plain text
	•	Clipboard share
	•	Tournament-ready CSV
	•	Designed to integrate with external rating systems
	•	Raw match data provided - ratings are calculated externally

⸻

🧑‍⚖️ Tournament Director (TD) Tool

The included TD.html page is built for tournament operators:
	•	Generate match links with player names and match length
	•	Create scannable QR codes for quick access
	•	Copy match URLs instantly
	•	Built-in collapsible help guide
	•	Can run as a standalone PWA
	•	Ideal for multi-table tournaments

⸻

📦 Progressive Web App (PWA)
	•	No install required - runs in any modern browser
	•	Installable on iOS, Android, tablets, and desktops
	•	Fully offline-capable via service workers
	•	Stable for live events and venues

⸻

📁 Project Files
	•	index.html - Main scoreboard app
	•	TD.html - Tournament Director tool
	•	manifest.json / manifestTD.json - PWA manifests
	•	service-worker.js / service-worker-td.js - Offline support
	•	Icons, logos, and assets for installs and branding

⸻

🧠 Design Philosophy

AHSS intentionally separates:
	•	Scoring logic
	•	Match data
	•	Visual presentation

This makes it easy to:
	•	Customize appearance without breaking rules
	•	Integrate with automation or rating pipelines
	•	Support different leagues with different needs

⸻

❤️ Credit

Created by Pete Lippincott
Built for the air hockey community, by the air hockey community.

Feedback, testing, and contributions are always welcome.

⸻

🏓 May the best mallet win.
