---
type: concept
system: "[[Physics]]"
source: "src/physics/PhysicsApi.cpp, src/script/SceneObservers.cpp"
---

`World::registerApi` delegates to `physics/PhysicsApi`: `applyImpulse`, `applyForce`, `applyTorque`, `raycast`, `gravity`, `setGravity`.

- The prelude wraps them so a raycast takes vectors and returns `{ node, position, normal, distance }` or `nil`, and every node proxy gets `applyImpulse(impulse, point?)`.
- Impulses and forces are **accumulators**, applied and cleared at the next step, so call order never matters.
- A node that isn't a `Body` raises a Lua error.

**Contacts:** `physics::ContactObserver` is an interface in `physics`, implemented in `script/SceneObservers`, installed by `LuaHost::boot` — physics never calls Lua directly. `World` compares this step's touching pairs with the last and fires `touched` and `touchEnded` on both nodes (Roblox's names). The node handed over is the `Body` if there is one, else the `Collider`. See [[SceneObserver]].
