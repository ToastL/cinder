---
type: module
system: "[[Platform]]"
source: "Source/platform/Assets.hpp"
---

`platform/Assets` is the only thing that turns a name into a path. Four roots:

- **`enginePath`** — engine data such as `Content/Fonts/`. `CINDER_ENGINE`, else `Engine/` next to the executable (packaged), else the repo root (`CINDER_ENGINE_DEFAULT`, baked in by [[cbt]]).
- **`shaderPath`** — compiled shaders. `CINDER_SHADERS`, else `Engine/Shaders/` next to the executable, else `Binaries/<Platform>/<Config>/Shaders/` (`CINDER_SHADERS_DEFAULT`), else `engineRoot()/Shaders`. Packaged beats the build dir on purpose.
- **`contentPath`** — `Content/`: scenes, `Sprite.texture`, `MeshPart.texture`, `engine.loadTexture`.
- **`sourcePath`** — `Source/`: the game's C++, which `cbt game` builds.

The loader picks the root, so a path never names it (`texture "Textures/box.png"`). An absolute path or one climbing out with `..` throws — a texture logs and draws white, and `game::loadTexture` throws. A path that differs in case from the disk logs `[assets] ... differs in case`, because macOS is case-insensitive and Linux isn't.

See [[Project folder]].
