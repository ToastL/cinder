---
type: concept
system: "[[Scripting]]"
source: "engine/lua/"
---

`LuaHost` loads three files from `engine/lua/` after `registerApi()` — `scene.lua` and `task.lua` need the `engine` table to exist:

- **`types.lua`** — `vec2`/`vec3`/`vec4`/`rgba` from one `vectype(keys)` factory, with arithmetic, `==`, `tostring`, `:length()`, `:dot()`, `:normalized()`, `:unpack()`, `vec3:cross()`. `rgba`'s `r/g/b/a` alias `x/y/z/w`. `vecSize(v)` tells a vector from a scalar.
- **`scene.lua`** — the `scene` global, [[Node proxies]], attributes, node signals.
- **`task.lua`** — `task.wait`/`spawn`/`delay`, `signal()`, the `stepped` and `rendered` signals, owners. `__step(dt)` (called by `ScriptHost::update`) steps the scheduler and fires `stepped`; `__render(alpha)` fires `rendered`.

Not hot-reloaded: editing the prelude needs a restart.
