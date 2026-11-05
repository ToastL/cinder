---
type: concept
system: "[[Physics]]"
source: "src/physics/World.cpp"
---

`World::step`, in order:

1. **Place** — a body whose node matrix isn't what physics last wrote was moved by something else; it teleports there, keeping velocity.
2. **Geometry** — each collider's world shape via `geometryOf`.
3. **Weigh** — mass split by volume, centre of mass, inertia. See [[Body]].
4. **Integrate velocities** — gravity, then damping.
5. **Detect** — see [[Collision detection]].
6. **Solve** — see [[Contact solver]].
7. **Rest** — see [[Sleeping and islands]].
8. **Advance** — integrate position and orientation, write the pose back.

`World` gathers `BodyState`s and owns traversal, integration, sleeping and contact events; it delegates contact prep, warm starting and impulses to `ContactSolver`. Begin the solver only after gathering — never grow the state vector while it's in use.
