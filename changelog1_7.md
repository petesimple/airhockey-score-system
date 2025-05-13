# Changelog

## v1.7 – miticlift (2025-05-12)

### ✨ New Features
- **Google Sheets Match Submission**  
  Match data is now posted directly to the `AHSS` Google Sheet using the `match` tab, aligned with the full schema (columns A–CZ).
- **FormData Backend Format**  
  Used `FormData` instead of JSON to conform to Apps Script's parameter-based `doPost(e)` method, avoiding CORS/redirect issues.
- **Game Score Simplification**  
  Sheet receives clean game scores (e.g., `7 - 4`) without player names embedded. In-app display remains unchanged for readability.
- **Skunk Flag Detection**  
  Automatically sets the `"Skunk"` column to `"Yes"` if any game ends in a `7 - 0`.

### 🛠 Improvements
- Added zero-padding to ensure `Set 1.1` through `Set 7.7` always exist in submissions.
- Added fallback handling to keep `Game Count` consistent with `p1.games` and `p2.games`.
- Submission URL and column structure fully decoupled from app UI—can now evolve independently.

### 🐞 Fixes
- Prevented empty or misformatted game data from reaching the backend.
- Ensured correct winner is logged even on final game call before form submission.

---

**Codename:** _"miticlift"_ – named for the epic comeback match between Goran and James Mitic.
