---
type: concept
system: "[[Build and shaders]]"
source: "Source/ThirdParty/README.md"
---

Vendored in `Source/ThirdParty/`, each with its licence and a `.Build.cpp`: glfw 3.4, glm 1.0.1, Vulkan-Headers (vulkan-sdk-1.4.357.0), VMA 3.3.0 and doctest 2.5.3. stb was replaced by our PNG decoder, and FreeType and HarfBuzz by `Text/TrueType` on 2026-11-26 ([[Decision - Own text stack]]). Versions and URLs are in `Source/ThirdParty/README.md`.

- glfw compiles Cocoa on macOS and X11 on Linux; Wayland is off.
- Nothing is downloaded; only the shader tools are installed — see [[Shader pipeline]].
- The vendored folders hold no build-system files; to update one, replace the folder, keep the licence and `.Build.cpp`, bump the README.
