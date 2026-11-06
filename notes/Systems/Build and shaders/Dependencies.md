---
type: concept
system: "[[Build and shaders]]"
source: "Source/ThirdParty/README.md"
---

Vendored in `Source/ThirdParty/`, each with its licence and a `.Build.cpp`: glfw 3.4, glm 1.0.1, Vulkan-Headers and volk (vulkan-sdk-1.4.357.0), VMA 3.3.0, stb, doctest 2.5.3, FreeType 2.14.3 and HarfBuzz 14.5.0. Versions and URLs are in `Source/ThirdParty/README.md`.

- FreeType compiles an explicit module list with zlib, bzip2, PNG, Brotli and HarfBuzz off, so it never finds Homebrew's copies.
- HarfBuzz is compiled from its single-file `src/harfbuzz.cc`.
- glfw compiles Cocoa on macOS and X11 on Linux; Wayland is off.
- Nothing is downloaded; only the shader tools are installed — see [[Shader pipeline]].
- The vendored folders hold no build-system files; to update one, replace the folder, keep the licence and `.Build.cpp`, bump the README.
