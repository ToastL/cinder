---
type: meta
last-update: 2026-11-06
---

How to check a change without a human at the keyboard. See also [[Testing]].

## Build into scratch, not the user's `Binaries/`
`Binaries/` and `Intermediate/` at the repo root are the user's; they build them themselves. Bootstrap a scratch `cbt` with `CINDER_BINARIES=<scratch>/cbt-bin ./Setup.sh`, then pass `--output <scratch>/out` to every `build`, `test`, `game` and `generate`. Say plainly when the user's `Binaries/` is stale.

- A stale binary in the scratch output fails in confusing ways (a font path from before a move); rebuild the target you run — `cbt test` builds only `editor` and `tests`.
- In zsh, `$X` holding a command with arguments is not word-split; drive multi-command loops through `bash -c`.
- `xcodebuild` with a project whose `projectDirPath` is the repo creates `build/` there: the generated project sets `SYMROOT`/`OBJROOT` into `Intermediate/Xcode/` for that reason.

## Drive and capture the editor
- `--input-script <file>` replays timed input in window points; `--capture-window <png>` writes the swapchain (UI included); `--capture` is the scene target only. See [[Input scripts]].
- A Retina capture is 2x: multiply points by 2 to find a pixel.
- Captures are deterministic run to run. For before/after, build the old commit into a scratch output and `cmp` the PNGs.
- `Samples/sandbox2d` only spawns while the left mouse is held, so an unattended capture is just the clear colour. Use sandbox3d or the selftest.
- A scratch project with a camera at `0 0 10`, fov 60 and a node at the origin puts the gizmo pivot at the Scene view's centre (`GIZMO_POINTS` = 100).
- python3 here has no PIL; a ~40-line zlib PNG decoder reads pixels when a colour needs checking.

## Selftest
Open `Tests/selftest` in the editor once (it builds the C++), then run `player Tests/selftest`: it quits by itself after `ALL PASS`. In the editor the test's quit only becomes Stop. See [[Selftest]].

## Renderer
Run with `DYLD_LIBRARY_PATH=/opt/homebrew/lib` so the validation layer loads. See [[Vulkan on Apple Silicon]].

## Metal memory leaks
- Measure with `footprint -p <pid>`, never `ps`/RSS: during the 2026-09-25 leak RSS showed 72 MB while footprint showed 55 GB.
- `OBJC_DEBUG_MISSING_POOLS=YES ./Binaries/macOS/Development/editor Samples/sandbox2d --frames N` prints one line per object autoreleased with no pool. Diff `grep -o "of class [A-Za-z]*" | sort | uniq -c` at two frame counts: flat counts are startup, growing counts are the leak.
- `lldb` with `breakpoint set -n objc_autoreleaseNoPool` gives a backtrace. Leaks on `com.apple.coreanimation.*` / `FramePacing` threads are QuartzCore's.
- An occluded window free-runs (~470 fps vs ~58) because `displaySyncEnabled` throttles only through `presentDrawable`. Per-frame leaks and `--frames` timings scale ~8x.
