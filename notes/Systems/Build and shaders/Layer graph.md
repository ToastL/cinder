---
type: concept
system: "[[Build and shaders]]"
source: "cmake/AssertLayers.cmake"
---

`src/` is acyclic and strictly layered; `architecture_layers` in `ctest` checks every first-party include, `architecture_checker` tests the checker.

```
reflect, platform      leaves
text                   -> platform
scene                  -> reflect
serial                 -> scene, reflect, platform
components             -> scene, reflect
physics                -> scene, reflect
ui/core → ui/framework → ui/widgets → ui/docking   (+ text, platform)
gfx/rhi                -> platform
gfx/vk | gfx/mtl       -> gfx/rhi, platform  (one per binary)
gfx/asset              -> gfx/rhi, scene
gfx/pass               -> gfx/asset, gfx/rhi, scene, platform
gfx                    -> gfx/pass, gfx/asset, gfx/rhi, ui/core, text, scene, platform
core                   -> all of the above
game                   -> core and below (what game code includes)
dev                    -> core and below (engine_dev)
player, editor, packager  executables
```

Nothing in a subdirectory includes its parent. `ui` never includes `scene`, `reflect` or `gfx`.

**Four placements that each kill a cycle:**
1. `Glfw`/`Window`/`Input` live in `platform`, not next to `Engine`.
2. `DrawList` lives in `scene`, so the tree never includes `gfx`.
3. `PropValue` lives in `scene`, so serializer and Properties each read it without an edge to one another.
4. `core/Module.hpp` (the `Registrar` and registration macros) lives in `core`, so `core/GameModule` can load a library without including `game`.

`game/Game.hpp` forward-declares `Engine`, so a game library needs only `src/` and glm on its include path. The generated `gfx/rhi/Backend.hpp` lives in the build tree and is invisible to the check.
