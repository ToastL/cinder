---
type: system
status: in-progress
layer: "platform, hosts"
source: "src/platform/Host.hpp (branch platform-hosts), src/hosts/"
started: 2026-09-26
last-update: 2026-09-26
---

## About

**What it is:** replacing GLFW with a virtual `platform::Host` and per-platform adapters, the way `gfx/rhi` declares what a backend defines. An approved 9-phase plan (2026-09-26); the user proposed the split themselves, mirroring Unreal's.

**Branch:** `platform-hosts`, one commit (`e9ca86a`, 2026-09-26): "refactor(platform): put windowing behind a platform::Host". Not merged; the current branch `option-platform` is based on `main`.

**The shape:**
- `platform/Host.hpp` declares the surface, sizes, `pump(wait)`, `time()`, close flag, clipboard, cursor and cursor lock, plus `createHost(const HostOptions&)`.
- Adapters in `src/hosts/<name>/` (layer → `platform` only). `CINDER_HOST` picks one per desktop executable.
- Hosts: **Cocoa** (macOS), **SDL3** (Linux), **UIKit** (iOS), **GameActivity** (Android), **headless** (tests). GLFW is a temporary first adapter, deleted in Phase 5. Consoles: a contract doc only.
- Hosts deliver input only on the main thread, inside `pump()` or their own callbacks. While the cursor is locked they report a virtual cursor accumulating raw deltas (GLFW's disabled-cursor semantics).

**What phase 1 did:** `Input` builds key names from `platform::keys` and asks the host to lock the cursor; volk bootstrap moved to `gfx/vk/Loader.cpp` and surface creation to `gfx/vk/Surface.cpp`; Metal takes its `CAMetalLayer` from the host and no longer links AppKit; `engine.time()` reads the host clock. Captures byte-identical to GLFW on mtl and vk. Linux X11/Wayland paths only compile on Linux, so they're untested.

**Headless host:** adds `selftest_headless` and `graphics_smoke_headless` to `ctest` — no window, fixed 1/60 s clock, byte-identical captures.

## Tasks
- [ ] Find the plan file — memory points to `~/.claude/plans/create-a-big-plan-frolicking-scroll.md`, which no longer exists (?)
- [ ] Decide which branch carries it forward
- [ ] Remaining phases: Cocoa, SDL3, delete GLFW (phase 5), UIKit, GameActivity

## Notes
- [[Decision - Windows deferred]], [[Decision - No CI for now]]. iOS before Android.
- Android also needs `platform/Assets` to read through `AAssetManager`; APK assets have no path. See [[Assets and paths]].
