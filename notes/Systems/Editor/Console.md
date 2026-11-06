---
type: module
system: "[[Editor]]"
source: "src/dev/panels/Console.hpp"
---

A log view. It installs the `LogSink` ([[Logging]]) in its constructor and clears it in its destructor. Lines queued by the sink join the panel in `update`, so nothing edits the widget tree during paint.

A Clear button, a line count, up to `MAX_LINES` lines; errors in `Color.ConsoleError`. `[build]`, `[module]` and `[package]` lines from [[Game code]] and the [[Editor packager]] arrive here.

Console commands registered by game code are an open item in `TODO.md`.
