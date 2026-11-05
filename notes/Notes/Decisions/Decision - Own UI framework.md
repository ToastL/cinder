---
type: decision
status: settled
date: 2026-09-22
---

**Decision:** the editor runs on a retained UI of our own (`src/text`, `src/ui/*`), shaped like Unreal's Slate. ImGui was deleted on 2026-09-23 (PR #4, `replace-imgui`).

**Why:** the same UI serves the editor and, later, game UI, and it can be driven and captured by the engine itself (`--input-script`, `--capture-window`).

**How to apply:** new editor UI is built with `ui::make<T>()`; never bring ImGui back. On macOS the editor is moving to SwiftUI — see [[Decision - Native macOS editor]]. See [[UI framework]].
