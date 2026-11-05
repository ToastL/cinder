---
type: concept
system: "[[Build and shaders]]"
source: "cmake/AssertLayers.cmake"
---

`src/` is acyclic and strictly layered; `architecture_layers` in `ctest` checks every first-party include, `architecture_checker` tests the checker.

```
reflect, platform      leaves
lua, text              -> platform
scene                  -> reflect
serial                 -> scene, reflect, platform
components             -> scene, reflect
physics                -> scene, reflect, lua
ui/core → ui/framework → ui/widgets → ui/docking   (+ text, platform)
gfx/rhi                -> platform
gfx/vk | gfx/mtl       -> gfx/rhi, platform  (one per binary)
gfx/asset              -> gfx/rhi, scene
gfx/pass               -> gfx/asset, gfx/rhi, scene, lua, platform
gfx                    -> gfx/pass, gfx/asset, gfx/rhi, ui/core, text, scene, platform, lua
script                 -> scene, reflect, gfx, physics, platform, lua
core                   -> all of the above
dev                    -> core and below (engine_dev)
```

Nothing in a subdirectory includes its parent. `ui` never includes `scene`, `reflect` or `gfx`.

**Five placements that each kill a cycle:**
1. `Glfw`/`Window`/`Input` live in `platform`, not next to `Engine`.
2. `DrawList` lives in `scene`, so the tree never includes `gfx`.
3. `LuaApi` lives in `lua`, so a pass binds its own functions without the script host.
4. `PropValue` lives in `scene`, so serializer, Lua and Properties each read it without an edge to one another.
5. `SceneObserver` is an interface in `scene`, implemented in `script` — see [[SceneObserver]].

`LuaHost` takes `(Scene&, Input&, Renderer&, physics::World&, quit)`, never `Engine&`, for the same reason. The generated `gfx/rhi/Backend.hpp` lives in the build tree and is invisible to the check.
