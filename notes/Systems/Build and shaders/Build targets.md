---
type: concept
system: "[[Build and shaders]]"
source: "CMakeLists.txt"
---

A plain build produces `engine`, `engine_dev`, `editor`, `player`, `packager`, `ui_gallery` and `tests`. `editor` depends on `player` and `packager`, so they always match it.

| Target | What |
|---|---|
| `engine` | the library: everything from `reflect` up to `game` |
| `engine_dev` | `src/dev/` — editor-only. See [[Decision - Dev tools as a link target]] |
| `editor` | the dev build; links `engine_dev`, exports the engine to game libraries. See [[Editor]] |
| `player` | the generic runtime; links `engine`, loads the project's game library |
| `packager` | stages a runnable game. See [[packager]] |
| `ui_gallery` | every widget live on one side of a splitter, every renderer primitive on the other; links `engine` only |
| `tests` | doctest; links `engine_dev` |
| `test_module`, `test_module_stale` | game libraries CMake builds for the module tests |
| `graphics_smoke` | optional, needs a window and GPU. See [[graphics_smoke]] |

A project's own C++ is never a CMake target: the editor builds it from `cinder-toolchain.json`. See [[Game code]].
