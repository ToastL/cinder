---
type: tool
system: "[[Testing]]"
source: "Tests/selftest/"
---

`Tests/selftest` is a project whose `SelfTest` node checks, from inside a game library, what game code relies on: scene editing, reflection, attributes, signals, timers, classes created by name at runtime, and physics.

```sh
./Binaries/macOS/Development/editor Tests/selftest --frames 2   # builds the project's C++
./Binaries/macOS/Development/player Tests/selftest              # prints ALL PASS
./Binaries/macOS/Development/packager Tests/selftest --config shipping
```

Needs a window, so it's not in `cbt test`. The `player` quits on its own when the test finishes. See [[Game code]].
