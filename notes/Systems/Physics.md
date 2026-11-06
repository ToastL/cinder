---
type: system
status: live
layer: "physics"
source: "Source/physics/"
started: 2026-09-20
last-update: 2026-09-21
---

## About

**What it is:** our own rigid-body engine ([[Decision - Own physics engine]]). A layer over `scene` and `reflect` with no GPU or window, so all of it runs headlessly in `cbt test`.

**Headers:** `Body`, `BodyState`, `Broadphase`, `Collide`, `Collider`, `ContactSolver`, `Geometry`, `Nodes`, `PhysicsApi`, `Pose`, `World`.

**How it runs:** `Engine` owns a `physics::World` and steps it in `Engine::update`, after `Scene::update`, before `input.consume()`. Edit mode never calls `update`, so **physics never runs while you edit**. `World::step` walks the scene every step instead of keeping a registry, in tree order with no `unordered_map` — so a scene steps identically every run (`Tests/physics_test` pins it bit for bit).

**2D:** `planar` on a body locks z travel and x/y turning. Same rule as the renderer, where `projection` is all that makes a scene 2D.

**Gravity:** `Config/Game.ini` `[Physics] gravity` (three numbers), default 9.81.

**Sample:** `Samples/physics` — floor, ramp, step, balls, capsules, a crate stack, and a `Dropper` that drops more balls. `--scene Scenes/planar.scene` opens the 2D half.

## Code
[[Body]], [[Collider]], [[Physics step]], [[Collision detection]], [[Contact solver]], [[Sleeping and islands]]. Game code calls `Body::applyImpulse`/`applyForce`/`applyTorque`, `game::raycast` and `game::world()`, and hears touches through `onTouched` or `Scene::connect`.

## Tasks
- [ ] Sensors and collision layers
- [ ] `unitsPerMeter` in `Game.ini` for pixel-scale projects
- [ ] Raycast through the broadphase tree
- [ ] Phase 5: joints, CCD, character controller, convex hulls
- [ ] Broadphase rebuilt from scratch every step
- [ ] Two capsules meet at one point (side-by-side pairs rock)
- [ ] Fast bodies tunnel (no CCD)
- [ ] Render interpolation — at `fixedHz` 60 on 120 Hz a falling body visibly steps

## Notes
