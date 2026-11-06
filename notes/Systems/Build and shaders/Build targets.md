---
type: concept
system: "[[Build and shaders]]"
source: "Source/editor/Editor.Target.cpp, Source/player/Player.Target.cpp, Tests/Tests.Target.cpp"
---

`cbt build` with no targets builds `editor`, `ui_gallery` and `tests`. The `editor` target requires `player` and `packager`, so they always match it.

| Target | What |
|---|---|
| `editor` | the dev build; root module `editor` → `dev`; exports the engine to game libraries. See [[Editor]] |
| `player` | the generic runtime; root module `player` → `core`, never `dev`; loads the project's game library |
| `packager` | stages a runnable game. See [[packager]] |
| `ui_gallery` | every widget live on one side of a splitter, every renderer primitive on the other; no `dev` |
| `tests` | doctest; depends on `dev`. See [[cbt test]] |
| `test_module`, `test_module_stale` | game-module targets for the module tests |
| `graphics_smoke` | only when named; needs a window and GPU. See [[graphics_smoke]] |

Engine modules compile once per configuration and are shared; a target's own modules compile with its defines. `dev` reaching only the editor, packager and tests is [[Decision - Dev tools as a link target]]. A project's own C++ is built by `cbt game`. See [[Game code]].
