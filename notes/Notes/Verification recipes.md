---
type: meta
last-update: 2026-11-05
---

How to check a change without a human at the keyboard. See also [[Testing]].

## Build in a scratch tree, not `build/`
`build/` is the user's tree; they rebuild it themselves. Verify in a scratch Ninja tree and say plainly when `build/` is stale.

A scratch tree configures offline by pointing FetchContent at the already-downloaded sources: `-DFETCHCONTENT_SOURCE_DIR_<NAME>=<repo>/build/_deps/<name>-src` for each dependency (names uppercased). Build the argument list in `bash -c` with an array — zsh doesn't word-split an unquoted `$ARGS`, so the overrides silently vanish.

## Drive and capture the editor
- `--input-script <file>` replays timed input in window points; `--capture-window <png>` writes the swapchain (UI included); `--capture` is the scene target only. See [[Input scripts]].
- A Retina capture is 2x: multiply points by 2 to find a pixel.
- Captures are deterministic run to run. For before/after, build the old commit in a scratch tree and `cmp` the PNGs.
- `samples/sandbox2d` only spawns while the left mouse is held, so an unattended capture is just the clear colour. Use sandbox3d or the selftest.
- A scratch project with a camera at `0 0 10`, fov 60 and a node at the origin puts the gizmo pivot at the Scene view's centre (`GIZMO_POINTS` = 100).
- python3 here has no PIL; a ~40-line zlib PNG decoder reads pixels when a colour needs checking.

## Selftest
Open `tests/selftest` in the editor once (it builds the C++), then run `player tests/selftest`: it quits by itself after `ALL PASS`. In the editor the test's quit only becomes Stop. See [[Selftest]].

## Renderer
Run with `DYLD_LIBRARY_PATH=/opt/homebrew/lib` so the validation layer loads. See [[Vulkan on Apple Silicon]].

## Metal memory leaks
- Measure with `footprint -p <pid>`, never `ps`/RSS: during the 2026-09-25 leak RSS showed 72 MB while footprint showed 55 GB.
- `OBJC_DEBUG_MISSING_POOLS=YES ./build/editor samples/sandbox2d --frames N` prints one line per object autoreleased with no pool. Diff `grep -o "of class [A-Za-z]*" | sort | uniq -c` at two frame counts: flat counts are startup, growing counts are the leak.
- `lldb` with `breakpoint set -n objc_autoreleaseNoPool` gives a backtrace. Leaks on `com.apple.coreanimation.*` / `FramePacing` threads are QuartzCore's.
- An occluded window free-runs (~470 fps vs ~58) because `displaySyncEnabled` throttles only through `presentDrawable`. Per-frame leaks and `--frames` timings scale ~8x.
