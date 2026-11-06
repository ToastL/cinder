---
type: tool
system: "[[Testing]]"
source: "Tests/ui_harness.hpp"
---

`Tests/ui_harness.hpp` drives a real `Application` headlessly with a `HeadlessPlatform` clock and clipboard. Everything in `text` and `ui` runs without a window, so layout, routing, popups, trees, docking and the editor panels are tested in `ctest`.
