---
type: concept
system: "[[Build and shaders]]"
source: "CMakeLists.txt"
---

A plain build produces `engine`, `engine_dev`, `editor`, `ui_gallery` and `tests` — **no game**.

| Target | What |
|---|---|
| `engine` | the library: everything from `reflect` up to `core` |
| `engine_dev` | `src/dev/` — editor-only. See [[Decision - Dev tools as a link target]] |
| `editor` | the dev build; links `engine_dev`. See [[Editor]] |
| `player` | the generic runtime; links `engine`. `EXCLUDE_FROM_ALL`, built on demand |
| `ui_gallery` | every widget live on one side of a splitter, every renderer primitive on the other; links `engine` only |
| `tests` | doctest; links `engine_dev` |
| `graphics_smoke` | optional, needs a window and GPU. See [[graphics_smoke]] |
| `package_game` | stages a runnable game. See [[package_game]] |

`ctest` builds `player` itself through a fixture, because `shipping_binary_is_clean` inspects it.
