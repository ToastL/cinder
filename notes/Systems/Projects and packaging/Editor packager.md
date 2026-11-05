---
type: module
system: "[[Projects and packaging]]"
source: "src/dev/Packager.hpp"
---

The editor's **Platforms** menu lists every platform in `dev/Packager` — macOS, Linux, Windows, iOS, Android — and packages the open project into `<project>/Saved/Builds/<Platform>/<name>/`.

- Runs `cmake --build <build> --target player`, then `PackageGame.cmake` with `HOST_PLATFORM` set, via `fork`/`exec` of `sh -c` in its own process group, read on a worker thread.
- Edit mode only. A dirty scene gets Save / Don't Save / Cancel first ("before packaging?"), because packaging copies `Content/` from disk.
- `cancel()` sends `SIGTERM` to the whole group; the destructor cancels instead of waiting. Closing mid-package asks **Stop and Close** / **Keep Packaging**.
- Output is queued and logged as `[package]` lines from `Toolbar::update`, on the main thread, so the console sink is never called off it.
- The build dir, its `cmake`, the player path and the script are baked into `engine_dev` at configure time.
- Only the host desktop platform is enabled; others are listed disabled with a tooltip. A project whose `targetPlatforms` omits a platform disables it too.
- **Adding a platform:** make `platformSupported` true and teach `PackageGame.cmake` its layout.

Tested by `packager_test`. See [[package_game]].
