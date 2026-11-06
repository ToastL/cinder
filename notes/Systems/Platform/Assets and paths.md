---
type: module
system: "[[Platform]]"
source: "src/platform/Assets.hpp"
---

`platform/Assets` is the only thing that turns a name into a path. Four roots:

- **`enginePath`** — fonts. `CINDER_ENGINE`, else `engine/` next to the executable (packaged), else the source tree's `engine/` (`CINDER_ENGINE_DEFAULT`).
- **`shaderPath`** — compiled shaders. `CINDER_SHADERS`, else `engine/shaders/` next to the executable, else `${CMAKE_BINARY_DIR}/shaders` (`CINDER_SHADERS_DEFAULT`), else `engineRoot()/shaders`. Packaged beats the build dir on purpose.
- **`contentPath`** — `Content/`: scenes, `Sprite.texture`, `MeshPart.texture`, `engine.loadTexture`.
- **`sourcePath`** — `Source/`: the game's C++, which the module builder walks.

The loader picks the root, so a path never names it (`texture "Textures/box.png"`). An absolute path or one climbing out with `..` throws — a texture logs and draws white, and `game::loadTexture` throws. A path that differs in case from the disk logs `[assets] ... differs in case`, because macOS is case-insensitive and Linux isn't.

See [[Project folder]].
