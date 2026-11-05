---
type: concept
system: "[[Scene]]"
source: "src/components/Builtins.cpp, src/physics/Nodes.cpp"
---

| Class | Base | Props | |
|---|---|---|---|
| `Group` | Spatial | — | moves its children |
| `Folder` | Node | — | organizes; breaks the transform chain |
| `MeshPart` | Spatial | `mesh`, `texture` | `cube`, `sphere`, `capsule` |
| `Sprite` | Spatial | `texture`, `size`, `color` | a quad in its local XY plane |
| `Camera` | Spatial | `projection`, `fov`, clip planes, `zoom`, `clearColor`, `virtualSize` | `projection` is all that makes a scene 2D |
| `Body` | Spatial | `motion`, `mass`, `gravityScale`, damping, `velocity`, `planar` | see [[Body]] |
| `Collider` | Spatial | `shape`, `size`, `friction`, `restitution` | see [[Collider]] |
| `Script` | Node | `file` | see [[Script node]] |

**Adding a class:** derive from `Node` or `Spatial`, declare props with `CINDER_NODE` ([[Props]]), register it in `components::registerBuiltins` (or `physics::registerNodes`). Serializer, Lua and editor need no change.
