---
type: concept
system: "[[Scripting]]"
source: "src/script/LuaHost.cpp, src/lua/LuaApi.hpp"
---

**Where to add a Lua function:** in the class that owns the state it touches — draw calls in `SpritePass`/`MeshPass`, camera calls in `Renderer`, physics in `PhysicsApi`. Not in `LuaHost`.

`LuaHost::registerApi()` builds the global `engine` table, delegates engine-wide calls (time, quit, log, input, textures) to `script/RuntimeApi`, then hands the `LuaApi` to `registerSceneApi`, `renderer.registerApi()` (forwarded to every pass) and `physics.registerApi()`.

- `lua_CFunction` can't capture, so each binding carries its receiver as a light-userdata upvalue: `api.bind("name", fn, &receiver)`, read with `LuaApi::context<T>(state)`.
- `LuaApi::optFloat` / `optInt` for optional numbers.
- `RuntimeContext` is a member of each host; no process-global binding context.
- `engine.quit()` only sets a flag: the player ends; the editor turns it into Stop.
