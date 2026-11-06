---
type: system
status: live
layer: "core"
source: "Source/core/"
started: 2026-09-09
last-update: 2026-09-25
---

## About

**What it is:** `Engine`, `GameLoop`, `LaunchOptions`, `ProjectConfig` and `ProjectDescriptor` — the top of the `engine` library.

**Frame flow:** `main` → `Glfw::acquire()` → `Engine(config)` → `engine.openScene(path)` → a loop. The player calls `GameLoop::tick` every iteration; the editor calls `PlaySession::tick`, which picks `tick`, `idle` or `step` ([[PlaySession]]).

`player/main.cpp` and `editor/main.cpp` duplicate their arg parsing on purpose. Don't factor it into `core`.

## Code
[[Engine]], [[GameLoop]]. Project loading: [[Game.ini]], [[Project descriptor]].

## Notes
