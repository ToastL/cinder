---
type: system
status: live
layer: "scene, components"
source: "Source/scene/, Source/components/"
started: 2026-09-09
last-update: 2026-11-06
---

## About

**What it is:** the world is one tree of nodes, the way Roblox and Godot build it. A node is exactly one class and things are made by nesting: a Box is a `MeshPart` with a `Spinner` inside. No components. Behaviour is a node class too, written in the project's C++ ([[Game code]]).

**Events:** the tree announces changes through `Scene::emit`; nodes hear them through their own hooks or `Scene::connect`, and schedule work with `Scene::after`/`every`. Both reach only started nodes, so nothing reacts in Edit mode — see [[Signals and timers]].

**Headers:** `scene/` — `Attributes`, `DrawList`, `Node`, `NodeTypes`, `PropValue`, `Scene`, `Spatial`, `Transform`, `View`. `Placeholder` (an unknown class, kept verbatim). `components/` — `Builtins`, `Camera`, `Folder`, `Group`, `MeshPart`, `Sprite`.

**Connections:** saved by [[Scene file format]], driven by [[Game code]], drawn through `DrawList` by the [[Renderer]], simulated by [[Physics]], edited by [[Explorer]] and [[Properties panel]].

## Code
[[Node]], [[Node classes]], [[Node lifecycle]], [[Signals and timers]], [[Attributes]]

## Tasks
- [ ] Node references — a prop type holding a node id
- [ ] Prefabs (`Scene::clone` is the start), tags
- [ ] A disabled node's timers still fire
- [ ] Runtime meshes cannot be named in a scene

## Notes
- 2026-11-06: signals, timers and the event hooks added for C++ game code; `Scene::clear` now drops every connection and timer. `scene::Placeholder` keeps an unknown class's fields so a scene saved without its game module loses nothing.
