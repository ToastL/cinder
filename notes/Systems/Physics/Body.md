---
type: class
system: "[[Physics]]"
source: "src/physics/Body.hpp"
---

What physics moves. Props: `motion` (`static | kinematic | dynamic`), `mass`, `gravityScale`, `linearDamping`, `angularDamping`, `velocity`, `angularVelocity`, `planar`.

- Velocity is a prop like any other: a scene file carries a starting velocity, Properties shows it live during Play, and `ball->setVelocity(0, 5, 0)` works from game code.
- A kinematic body takes its pose from its node and derives velocity from how far it moved.
- The pose is written back as local position and Euler angles in `Transform::local()`'s order (Y, X, Z). The world keeps a quaternion, so a tumble through ±90° pitch simulates exactly even though Properties' angles flip.
- Mass is split between its colliders by volume; it turns about its **centre of mass**, with a full inertia tensor via the parallel axis theorem.
