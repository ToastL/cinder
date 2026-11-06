---
type: tool
system: "[[Testing]]"
source: "Tests/graphics/smoke.cpp"
---

Optional, needs a window and a GPU:
```sh
./Binaries/macOS/cbt build graphics_smoke
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./Binaries/macOS/Development/graphics_smoke
```
Runs the editor's own UI with injected input: clicks a node in the Explorer, drags a move gizmo, undoes and redoes with ⌘Z, saves a scene, plays and stops with ⌘P, cycles Play/Pause/Step/Resume/Stop and a game-side quit, re-docks the console, resizes the window and scene targets, and captures both window and composited view.

Takes coordinates from `Application::grid().geometryOf(widget)`, not hard-coded points. Output stays in `Intermediate/macOS/<Config>/graphics-smoke/`; never saves over a sample.
