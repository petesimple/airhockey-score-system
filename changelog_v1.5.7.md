# Air Hockey Score System - Changelog

## Version 1.5.7 (streaming)

### ✨ New
- Added styling and layout improvements to the skinny scoreboard to ensure proper centering.
- `#skinnyScoreboard` now uses `flex-direction: column` and `align-items: center` to avoid left alignment issues in streaming mode.

### 🛠 Fixes
- Fixed layout bug where the skinny scoreboard was pushed to the left when toggled on.
- Adjusted `toggleSkinnyScoreboard()` function to explicitly reset flexbox properties when showing the skinny scoreboard.

### 🔧 Misc
- Version label updated to `ver1.5.7 – (streaming)`
