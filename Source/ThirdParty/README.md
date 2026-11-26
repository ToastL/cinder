# Third-party code

Every dependency is vendored here and built by `cbt` from its own `<Name>.Build.cpp`. Nothing is
fetched at build time, and none of these libraries' own build systems is used.

| Folder | Library | Version | Upstream | Licence |
|---|---|---|---|---|
| `glfw/` | GLFW | 3.4 | https://github.com/glfw/glfw | zlib (`LICENSE.md`) |
| `glm/` | GLM | 1.0.1 | https://github.com/g-truc/glm | MIT (`copying.txt`) |
| `vulkan-headers/` | Vulkan-Headers, C headers only | vulkan-sdk-1.4.357.0 | https://github.com/KhronosGroup/Vulkan-Headers | Apache-2.0 / MIT (`LICENSE.md`, `LICENSES/`) |
| `vma/` | Vulkan Memory Allocator | v3.3.0 | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | MIT (`LICENSE.txt`) |
| `doctest/` | doctest | v2.5.3 | https://github.com/doctest/doctest | MIT (`LICENSE.txt`) |

How each one is built:

- **GLFW** compiles its Cocoa sources on macOS and its X11 sources on Linux. Wayland is off until
  it can be tested.
- **VMA**'s implementation is `vma/VmaImplementation.cpp`.
- **The Vulkan modules** are used only by the `vk` backend.

To update a library: replace its folder with the upstream tag, keep its licence files and its
`.Build.cpp`, change the version in this table, and run `cbt test`.
