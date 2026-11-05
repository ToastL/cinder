---
type: module
system: "[[Platform]]"
source: "src/platform/Assets.hpp"
---

`platform/Assets` is the only thing that turns a name into a path. Four roots:

- **`enginePath`** — prelude and fonts. `CINDER_ENGINE`, else `engine/` next to the executable (packaged), else the source tree's `engine/` (`CINDER_ENGINE_DEFAULT`).
- **`shaderPath`** — compiled shaders. `CINDER_SHADERS`, else `engine/shaders/` next to the executable, else `${CMAKE_BINARY_DIR}/shaders` (`CINDER_SHADERS_DEFAULT`), else `engineRoot()/shaders`. Packaged beats the build dir on purpose.
- **`contentPath`** — `Content/`: scenes, `Sprite.texture`, `MeshPart.texture`, `engine.loadTexture`.
- **`sourcePath`** — `Source/`: a `Script`'s `file`.

The loader picks the root, so a path never names it (`file "bobber.lua"`, `texture "Textures/box.png"`). An absolute path or one climbing out with `..` throws — a texture logs and draws white, a script raises a Lua error. A path that differs in case from the disk logs `[assets] ... differs in case`, because macOS is case-insensitive and Linux isn't.

See [[Project folder]].
