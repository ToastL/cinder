---
type: concept
system: "[[Game code]]"
source: "Source/dev/ModuleHost.hpp, Source/dev/ModuleHost.cpp, Source/dev/EditorApplication.cpp"
---

How the editor swaps a project's library without restarting.

**Build** — the editor runs `cbt game` synchronously before the scene opens, so its classes exist when the scene loads. After that, **Build** on the toolbar and **⌘B** run it in the background through `dev/Process`; `ModuleHost::update` relays the output as `[build]` lines on the main thread, counts compiles and errors, and marks a reload pending when the library was relinked. Cancel sends `SIGTERM` to `cbt`'s whole process group. See [[cbt]].

**When** — `dev/EditorApplication.cpp`'s loop applies a pending reload between frames, only in Edit mode and only when `app.isInteracting()` is false. During Play it waits for Stop.

**The swap** (`ModuleHost::swap`):
1. Copy the library to `Saved/Binaries/Loaded/<name>-<n>.dylib` — `dlopen` caches by path.
2. `GameModule::open` the copy: `dlopen` plus the BuildId check.
3. Save the scene to text, `Scene::clear()` it — the old classes' destructors run while their code is still loaded.
4. Destroy the old `GameModule` (`NodeTypes::remove` per class, then `dlclose`) and delete its copy.
5. `enlist` the new module's classes and load the text back.

A failure at 1 or 2 logs `[module]` and leaves the old library loaded and the scene untouched.

**What survives** — selection, undo history and the Explorer's open rows, because each is keyed by id or text. `Properties::rebuild` runs after a swap because its rows hold `PropDef`s from the old library. Connections and timers do not survive ([[Signals and timers]]), and statics in the library reset — here and nowhere else (Play doesn't reset them).

**Housekeeping** — `ModuleHost`'s constructor deletes `Saved/Binaries/Loaded/`; its destructor clears the scene before unloading.

**Tested by** — `Tests/hot_reload_test`, the `hot_reload` check in [[cbt test]]: a real two-version library built through `cbt game` and swapped.
