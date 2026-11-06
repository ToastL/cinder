---
type: system
status: live
layer: "platform"
source: "Source/platform/"
started: 2026-09-09
last-update: 2026-11-06
---

## About

**What it is:** the leaf every layer may reach for: logging, the asset-path resolver, file I/O, GLFW windowing and input, and input-script replay. Neither `Log.hpp` nor `Assets.hpp` includes anything from the engine, so the edges cost nothing.

**Headers:** `Assets`, `Cursor`, `Files`, `Glfw`, `Input`, `InputEvent`, `InputScript`, `Log`, `Window`.

**Connections:** `Input` feeds the game ([[Engine]]) and the editor UI ([[UI input routing]]). `Assets` resolves every path in [[Projects and packaging]]. Windowing is being replaced by [[Platform hosts]].

## Code
[[Assets and paths]], [[Input]], [[Input scripts]], [[Logging]], [[Vulkan on Apple Silicon]]

## Tasks
- [ ] The packaged player finds itself through `argv[0]`, so a `PATH` launch misses `Engine/` and `Project/`
- [ ] Settings/save-data location per OS

## Notes
- `platform/Files` is the shared binary-mode text I/O for project loading, scenes, game-code builds and history.
