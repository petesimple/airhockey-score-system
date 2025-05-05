# 🛠️ Air Hockey Score System – Fix & Polish Update (v1.4.5)

## 🎨 Visual & Layout Enhancements
- **Timer Positioning Fixed**
  - The match timer now **always appears directly below the score display**, even when the scoreboard is locked to the top.
  - This ensures consistent placement for both live use and OBS streaming overlays.

- **Tighter Scoreboard Spacing**
  - Introduced a `margin-bottom` to `#scoreBoard` for better separation from the timer.
  - Reduced reliance on hardcoded `margin-top` in `#timer` to allow more flexible layout control.

## 🧰 Stability & Usability Fixes
- **Scoreboard Lock Mode Improvements**
  - Fixed layout issues where the timer and controls would shift or get obscured when the scoreboard was pinned to the top.
  - Now behaves predictably with consistent padding and visibility for all components.

- **Setup Fade Reset**
  - After resetting the match, all setup elements (logo, name inputs, controls) **reappear using flex display**, maintaining alignment and styling.