---
type: meta
last-update: 2026-11-05
---

What landed when, from git history. Newest first.

- **2026-09-26** — [[Editor packager]]: the Platforms menu packages the open project. On `platform-hosts`: windowing moved behind `platform::Host` ([[Platform hosts]]). Decisions: [[Decision - Native macOS editor]] layout, [[Decision - Windows deferred]], [[Decision - No CI for now]].
- **2026-09-25** — Metal autorelease-pool leak fixed in `Presenter` (see [[Verification recipes]]). Drag and drop and input handling. PR #5 (`switch-backend`) merged.
- **2026-09-23** — Shaders moved from GLSL to HLSL through DXC ([[Shader pipeline]]). The `gfx/rhi` [[Backend seam]], then the [[Metal backend]], pixel-identical to Vulkan. ImGui deleted; PR #4 (`replace-imgui`) merged.
- **2026-09-22** — [[Text shaping]] with FreeType and HarfBuzz, the UI renderer, the retained widget core, popups, menus, value editors, trees and [[Docking]] ([[UI framework]]).
- **2026-09-21** — Runtime API split out of `LuaHost`. [[Decision - Script nodes stay]], [[Decision - Project browser deferred]].
- **2026-09-20** — [[Physics]] started.
- **2026-09-16** — [[Transform gizmos]]. [[Decision - Own physics engine]], [[Decision - Fly keys always on]].
- **2026-09-15** — [[Undo history]].
- **2026-09-11** — Scene fully representable as data; Lua behaviour reload; logging rework; the dev tools target.
- **2026-09-09** — First commit: text serialization for scene objects.
