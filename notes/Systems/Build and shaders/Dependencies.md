---
type: concept
system: "[[Build and shaders]]"
source: "cmake/Dependencies.cmake"
---

All fetched and pinned in `cmake/Dependencies.cmake`: glfw, glm, lua, VMA, stb, volk, Vulkan-Headers, doctest, FreeType and HarfBuzz.

- FreeType is built with zlib, bzip2, PNG, Brotli and HarfBuzz off, so it never finds Homebrew's copies.
- HarfBuzz is compiled from its single-file `src/harfbuzz.cc`, not its community CMake build.
- Nothing needs installing except the shader tools — see [[Shader pipeline]].
- Offline scratch trees reuse `build/_deps/*-src` through `FETCHCONTENT_SOURCE_DIR_<NAME>`. See [[Verification recipes]].
