---
type: system
status: live
layer: "core, game, dev"
source: "src/game/, src/core/GameModule.hpp, src/core/Module.hpp, src/dev/ModuleBuilder.hpp, src/dev/ModuleHost.hpp"
started: 2026-11-06
last-update: 2026-11-06
---

## About

**What it is:** C++ game code. A project's `Source/` defines node classes that the editor compiles into `Saved/Binaries/<name>.dylib`, loads before the scene opens, and hot-reloads between frames (⌘B). The player loads `Binaries/<Platform>/<name>.dylib`. See [[Decision - C++ game code]].

**Pieces:**
- `game/Game.hpp` — what modules include: node classes plus free functions (input, time, drawing, raycast, random) over `Engine::current()`.
- `core/Module.hpp` — `CINDER_REGISTER`, `CINDER_MODULE_ENTRY`, `Registrar`.
- `core/GameModule` — dlopen, ABI check, register, unregister before dlclose.
- `dev/Toolchain` + `cinder-toolchain.json` — compiler, flags, includes, defines from CMake.
- `dev/ModuleBuilder` — parallel, incremental (depfiles), `compile_commands.json`.
- `dev/ModuleHost` — editor build/reload; swap = copy, open, save scene, clear, unload, enlist, load.
- `scene::Placeholder` — unknown classes survive a failed build byte for byte.
- `Scene::connect`/`after`/`every` + node event hooks: events and timers owned by the scene.

**Tests:** `game_module_test` (CMake-built test module), `hot_reload` ctest entry (real clang), C++ selftest project.

## Notes
- 2026-11-06: Built in one session on branch `cpp-gameplay`, 7 phases. Pitfall found: Apple libc++ hashes a hidden `type_info` by name and the engine's by address → `NodeTypes` keys by mangled name.
- A library built against older engine headers fails `dlopen` (missing symbol) → placeholders; reopen in the editor to rebuild.
