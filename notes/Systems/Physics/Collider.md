---
type: class
system: "[[Physics]]"
source: "src/physics/Collider.hpp, src/physics/Geometry.hpp"
---

A shape: `shape` (`box | sphere | capsule`), `size`, `friction`, `restitution`.

- **Belongs to its nearest `Body` ancestor** (Unreal's welding rule): every shape under one body is one rigid compound. A collider with no body above it is static — a floor is a `MeshPart` with a `Collider` inside.
- Material lives on the shape, because a bodiless floor still needs friction. A pair combines as `sqrt(fA*fB)` and `max(eA, eB)`.
- `size` is the box the shape fits in: a sphere takes the largest axis; a capsule stands along its Y, as wide as max(x, z) and as tall as y (`1 2 1` is a capsule of length 2; `1 1 1` collapses to a sphere). Default `1 1 1` box = the unit cube a `MeshPart` draws.
- `geometryOf` gives the world shape. The repo's `CLAUDE.md` says `dev/Gizmos` draws collider wireframes from it, but no `src/dev` file calls `geometryOf` and there's no `dev/Gizmos` (?).
