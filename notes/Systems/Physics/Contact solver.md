---
type: module
system: "[[Physics]]"
source: "src/physics/ContactSolver.hpp"
---

Sequential impulses, `ITERATIONS` passes of friction then normal.

- Accumulated normal impulse clamped to push only; two tangent impulses clamped to Coulomb's `friction * normal`.
- **Warm starting:** impulses carry across steps by feature id — what makes a stack stand rather than sag.
- Restitution above `BOUNCE_THRESHOLD` goes into the velocity pass.
- **Split impulse:** penetration is corrected in a second pass of pseudo-velocities, so pushing apart never adds momentum. Baumgarte in the velocity pass shook a ten-crate stack apart.

Owns its contacts and carried impulses, works on a span of the step's body states, exposes read-only contacts for [[Sleeping and islands]].
