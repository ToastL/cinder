---
type: meta
last-update: 2026-11-05
---

A map of what's open, by area. `TODO.md` is authoritative and has the detail; tick items there.

**Done:** foundations, the scene model, scripting, serialization, the Edit/Play split, the editor shell (own UI, docking, Explorer, Properties, Scene view, gizmos, undo), physics phases 1–4, the backend seam and the Metal backend.

## Editor ([[Editor]])
- Explorer multi-select, inline rename, copy/paste
- Asset- and node-reference widgets in Properties
- Gizmos: world/local rotate rings, grid snapping, gizmos during Play, multi-selection
- Picking against real mesh bounds; Scene view 2D mode
- Undo via diffed snapshots; console `file:line` links, filter, search
- Layout persisted to `Saved/`; Project Settings panel; scene open/new/save-as
- Project browser — [[Decision - Project browser deferred]]

## UI ([[UI framework]])
- Font fallback (symbol font for ⌘⇧P); floating panels; game UI as `.widget` assets

## Play mode
- Keep a change made during Play

## Serialization ([[Serialization]])
- Node references, prefabs, tags

## Assets
- GUIDs and `.meta` sidecars, asset database, import pipeline, refcounting and unload, texture/mesh/shader hot reload, mesh import, asset browser

## Gameplay
- Physics: sensors and layers, `unitsPerMeter`, raycast via the tree, phase 5 (joints, CCD, character controller, convex hulls) — see [[Physics]]
- Render interpolation (`alpha` is unused)
- Audio, animation, screen-space UI, particles, scene queries, timers/event bus

## Ship a game ([[Projects and packaging]])
- Asset archive, per-OS save-data location, crash handler + log file

## Rendering ([[Renderer]])
- Lights and materials, shadows, sorting, culling, mipmaps, anisotropy, MSAA, post chain, instancing, bindless, compute

## Platforms
- [[Platform hosts]] (in progress), [[macOS editor]] (planned), Android (GLFW has no backend; APK assets have no paths). Windows deferred.

## Out of scope
Feature parity with Roblox/Unity/Unreal, consoles, networking, visual scripting, a second scripting language, ray tracing.
