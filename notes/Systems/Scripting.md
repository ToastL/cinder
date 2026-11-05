---
type: system
status: live
layer: "lua, script"
source: "src/lua/, src/script/, engine/lua/"
started: 2026-09-09
last-update: 2026-09-21
---

## About

**What it is:** Lua 5.4 in Roblox's shape. Game code lives in `Script` nodes; a script's file runs top to bottom once when the node starts, with `script.parent` the node it sits in. Per-frame work is a `stepped:connect(fn)` handler; the top level may `task.wait`. Per-node settings are [[Attributes]]. Lua errors are caught and printed.

```lua
local box = script.parent
stepped:connect(function()
    local h = 1 + math.sin(engine.time() * box:getAttribute("rate") + box:getAttribute("phase"))
    box.scale = vec3(1, h, 1)
end)
```

**Headers:** `lua/` — `LuaApi`, `LuaCalls`, `LuaSource`. `script/` — `LuaHost`, `LuaProps`, `RuntimeApi`, `SceneApi`, `SceneObservers`, `Script`, `ScriptHost`.

**Connections:** `LuaHost` takes `(Scene&, Input&, Renderer&, physics::World&, quit)` — never `Engine&`. Each Play boots a fresh `lua_State` ([[PlaySession]]). Tested by [[Lua selftest]].

## Code
[[Script node]], [[Lua prelude]], [[Node proxies]], [[Lua bindings]], [[Task ownership]], [[Hot reload]], [[Lua error safety]]

## Tasks
- [ ] Scripts can't share code — no `ModuleScript`/`require`
- [ ] The prelude isn't watched (editing it needs a restart)
- [ ] `poll()` stats every read script every frame

## Notes
- The scripting model is the child `Script` node — [[Decision - Script nodes stay]].
