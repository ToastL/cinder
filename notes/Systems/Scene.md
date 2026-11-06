---
type: system
status: live
layer: "scene, components"
source: "src/scene/, src/components/"
started: 2026-09-09
last-update: 2026-09-21
---

## About

**What it is:** the world is one tree of nodes, the way Roblox and Godot build it. A node is exactly one class and things are made by nesting: a Box is a `MeshPart` with a `Spinner` inside. No components. Behaviour is a node class too, written in the project's C++ ([[Game code]]).

**Headers:** `scene/` — `Attributes`, `DrawList`, `Node`, `NodeTypes`, `PropValue`, `Scene`, `Spatial`, `Transform`, `View`. `components/` — `Builtins`, `Camera`, `Folder`, `Group`, `MeshPart`, `Sprite`.

**Connections:** saved by [[Scene file format]], driven by [[Game code]], drawn through `DrawList` by the [[Renderer]], simulated by [[Physics]], edited by [[Explorer]] and [[Properties panel]].

## Code
[[Node]], [[Node classes]], [[Node lifecycle]], [[Attributes]]

## Tasks
- [ ] Node references — a prop type holding a node id
- [ ] Prefabs (`Scene::clone` is the start), tags
- [ ] A disabled node's timers still fire
- [ ] Runtime meshes cannot be named in a scene

## Notes
