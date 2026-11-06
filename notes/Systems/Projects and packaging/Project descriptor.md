---
type: format
system: "[[Projects and packaging]]"
source: "src/core/ProjectDescriptor.hpp"
---

The `.cinder` file: JSON with Unreal's `.uproject` fields in camelCase. Says what the project *is*; `Config/` says how it's set up.

- **`fileVersion`** — must be `ProjectDescriptor::FILE_VERSION` (1).
- **`engineAssociation`** — the cinder version it was made with; a mismatch with `CINDER_VERSION` (CMake's `PROJECT_VERSION`) warns. Empty = lives in the engine tree (the samples).
- **`targetPlatforms`** (`macOS`, `Linux`, `Windows`; empty = all) and **`preBuildSteps`** / **`postBuildSteps`** (platform → command lines) — read by [[packager]].
- **`category`**, **`description`** — for a project picker.
- **`modules`**, **`plugins`**, **`additionalRootDirectories`**, **`additionalPluginDirectories`**, **`disableEnginePluginsByDefault`** — parsed, type-checked, saved, read by nothing yet.

`save` writes Unreal's layout (tabs, first four keys always, the rest when set); `scene_files_test` holds the samples to it. Wrong types or a nameless module/plugin are errors naming the key. Parsed by `serial/Json` ([[Archive]]).
