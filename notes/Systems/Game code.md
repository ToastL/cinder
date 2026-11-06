---
type: system
status: live
layer: "core, game, dev"
source: "Source/game/, Source/core/GameModule.hpp, Source/core/Module.hpp, Source/dev/GameBuild.hpp, Source/dev/ModuleHost.hpp"
started: 2026-11-06
last-update: 2026-11-06
---

## About

**What it is:** C++ game code. A project's `Source/` defines node classes that the editor compiles into `Saved/Binaries/<name>.dylib`, loads before the scene opens, and hot-reloads between frames (⌘B). The player loads `Binaries/<Platform>/<name>.dylib`. See [[Decision - C++ game code]].

**Pieces:**
- `game/Game.hpp` — what modules include: node classes plus free functions (input, time, drawing, raycast, random) over `Engine::current()`.
- `core/Module.hpp` — `CINDER_REGISTER`, `CINDER_MODULE_ENTRY`, `Registrar`.
- `core/GameModule` — dlopen, BuildId check, register, unregister before dlclose.
- `cbt game <project>` — builds `Source/` (default module `Project.<name>`, or the project's own rules), incremental, `compile_commands.json`. See [[cbt]].
- `dev/GameBuild` — finds `cbt` next to the binaries and spells its command lines.
- `dev/ModuleHost` — runs `cbt game` through `dev/Process` and swaps the library between frames. See [[Hot reload]].
- `scene::Placeholder` — unknown classes survive a failed build byte for byte.
- `Scene::connect`/`after`/`every` + node event hooks: events and timers owned by the scene. See [[Signals and timers]].

**Tests:** `game_module_test` (cbt-built test modules), the `hot_reload` check in [[cbt test]], the C++ selftest project.

## Notes
- 2026-11-06: Built in one session on branch `cpp-gameplay`, 7 phases. Pitfall found: Apple libc++ hashes a hidden `type_info` by name and the engine's by address → `NodeTypes` keys by mangled name.
- A library built against older engine headers fails `dlopen` (missing symbol) → placeholders; reopen in the editor to rebuild.
- 2026-11-06: built by [[cbt]] now. The engine's BuildId is in every game compile command, so stale game code recompiles; a project named like an engine module (the `physics` sample) once compiled the engine's physics instead — project modules are `Project.<name>` since.
