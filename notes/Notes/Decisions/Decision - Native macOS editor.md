---
type: decision
status: settled
date: 2026-09-25
---

**Decision:** the macOS editor becomes a real Swift/SwiftUI app (Liquid Glass, native menu bar, native panels) with the engine embedded as a library. Linux keeps the own-UI editor. Two front ends, on purpose.

**Why:** the user wants the Mac build to feel like a Mac app. Claude recommended one UI everywhere; they rejected it: "no, i want it to look like a MacOS app, so i want swift for the MacOS side with the engine integrated inside of it."

**Choices (2026-09-26):** Xcode-shaped layout (sidebar Explorer, viewport, `.inspector` Properties, bottom console, no docking); macOS 26 minimum for the editor; Metal only.

**How to apply:** don't re-pitch a single UI. See [[macOS editor]].
