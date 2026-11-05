---
type: decision
status: settled
date: 2026-09-16
---

**Decision:** cinder's physics is written from scratch in `src/physics/`. No Jolt, Box2D, Bullet or PhysX.

**Why:** when physics came up as the next roadmap item and Claude started designing around Jolt, the user stopped it: "i actually want to build my own physics engine". It matches the [[Decision - Own UI framework]] stance: core systems are our own.

**How to apply:** published algorithms and reference code (Catto's GDC talks, Box2D-lite) are fine to learn from; linking a physics library is not. See [[Physics]].
