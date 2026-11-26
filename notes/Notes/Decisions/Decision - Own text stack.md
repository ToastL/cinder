---
type: decision
status: settled
date: 2026-11-26
---

**Decision:** drop FreeType and HarfBuzz and read fonts with our own code.

**User's words:** "get rid of harfbuzz and freetype". This followed a review of which ThirdParty libraries to remove, where these two were about 16 MB and 330k of the vendored lines but served only three Roboto `.ttf` files.

**Accepted trade-offs (in the approved plan):** no GSUB (so no `fi` ligature), no mark positioning or RTL, no hinting (at 1x the text is slightly softer than FreeType's light autohint).

**How to apply:** see [[Text shaping]]. Don't propose bringing either library back. Extend `Text/TrueType` instead; the open items are in `TODO.md`.
