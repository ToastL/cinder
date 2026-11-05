---
type: hub
---

A from-scratch C++20 game engine: Vulkan and Metal renderers, Lua 5.4 scripting, its own UI, physics and editor. The target is a Unity/Unreal-shaped workflow — select a node, edit its fields, hit Play, hit Stop, land back where you started. Repo: `ToastL/cinder` on GitHub, main branch `main`.

## Engine (`engine` library)
- [[Build and shaders]] — CMake, pinned dependencies, DXC shader pipeline, the layer graph
- [[Platform]] — logging, asset paths, input, the window
- [[Reflection]] — `CINDER_PROP` and the prop system
- [[Scene]] — the node tree, lifecycle, attributes
- [[Serialization]] — scene text, INI, JSON, `Archive`
- [[Renderer]] — `gfx`: the RHI seam, Vulkan and Metal backends, passes, the UI renderer
- [[Physics]] — our own rigid-body engine
- [[UI framework]] — `text` + `ui`: a retained, Slate-shaped UI
- [[Scripting]] — Lua host, prelude, `Script` nodes, hot reload
- [[Core and game loop]] — `Engine`, `GameLoop`, project config

## Tools and shipping
- [[Editor]] — `engine_dev`: panels, Scene view, gizmos, undo, Play/Stop
- [[Projects and packaging]] — project folders, the `.cinder` descriptor, `package_game`
- [[Testing]] — `ctest`, the Lua selftest, `graphics_smoke`

## In flight
- [[Platform hosts]] — GLFW → `platform::Host` adapters (branch `platform-hosts`)
- [[macOS editor]] — SwiftUI editor over a C ABI (planned)

## How we work
- [[Conventions]] — zero comments, trailing underscores, `to_chars`, points vs pixels
- [[Verification recipes]] — scratch build trees, input scripts, captures
- [[Roadmap]] — what's open, by area. `TODO.md` is authoritative.
- [[Timeline]] — what landed when
- [[Working with Claude]] — efficiency tips. Vault rules for Claude are in `CLAUDE.md`.
- Decisions: see `Decisions.base` or `Notes/Decisions/`.
- `(?)` marks something unverified. Keep it in the note text, never in properties.

## Open questions
- [[Platform hosts]] is one commit in on `platform-hosts`; `option-platform` (current) is based on `main`. Which branch carries the hosts work forward?
- [[macOS editor]] contradicts `TODO.md`'s "no second UI system" line — not rewritten yet.
- Branches `edit-play-mode`, `redesign`, `replace-imgui` and `switch-backend` are merged into `main` but still exist locally and on origin. `add-custom-ui` is not merged.
- No CI ([[Decision - No CI for now]]).
