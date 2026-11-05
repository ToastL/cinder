---
type: system
status: planned
layer: "capi"
source: "src/capi/ (planned)"
started: 2026-09-25
last-update: 2026-09-26
---

## About

**What it is:** a native Swift/SwiftUI editor for macOS with the C++ engine embedded as a library. Linux keeps the own-UI [[Editor]]. See [[Decision - Native macOS editor]].

**Layout (decided 2026-09-26):** Xcode-shaped — sidebar Explorer, viewport, `.inspector` Properties, bottom console, no docking. macOS 26 minimum for the editor (no Liquid Glass fallbacks); the player can target older macOS. Metal only (`CINDER_BACKEND=mtl`).

**Bridge:** a hand-written flat C ABI in `src/capi/` (`cinder.h`, `cn_` prefix) over `engine_dev` — not Swift's C++ interop.

**Invariant:** `CINDER_PROP` still needs zero editor code. The Swift inspector is one generic view per `PropType`/`PropHint`, never a view per node class. See [[Props]], [[Properties panel]].

## Tasks
- [ ] Rewrite `TODO.md`'s "no second UI system" line (written against Qt), which this contradicts
- [ ] Depends on [[Platform hosts]] (a Cocoa host the Swift app can embed)

## Notes
- Keep the own-UI editor working throughout: `--input-script`, `--capture-window` and [[graphics_smoke]] are the only automated editor verification.
