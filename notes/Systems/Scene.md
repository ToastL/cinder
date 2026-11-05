---
type: system
status: live
layer: "scene, components"
source: "src/scene/, src/components/"
started: 2026-09-09
last-update: 2026-09-21
---

## About

**What it is:** the world is one tree of nodes, the way Roblox and Godot build it. A node is exactly one class and things are made by nesting: a Box is a `MeshPart` with a couple of `Script`s inside. No components. A node class exists only when the engine must know about it (draw, simulate, organize); behaviour is a [[Script node]].

**Headers:** `scene/` — `Attributes`, `DrawList`, `Node`, `NodeTypes`, `PropValue`, `Scene`, `Spatial`, `Transform`, `View`. `components/` — `Builtins`, `Camera`, `Folder`, `Group`, `MeshPart`, `Sprite`.

**Connections:** saved by [[Scene file format]], driven from Lua through [[Node proxies]], drawn through `DrawList` by the [[Renderer]], simulated by [[Physics]], edited by [[Explorer]] and [[Properties panel]].

## Code
[[Node]], [[Node classes]], [[Node lifecycle]], [[Attributes]], [[SceneObserver]]

## Tasks
- [ ] Node references — a prop type holding a node id
- [ ] Prefabs (`Scene::clone` is the start), tags
- [ ] A disabled parent does not stop its child scripts
- [ ] Runtime meshes cannot be named in a scene

## Notes
