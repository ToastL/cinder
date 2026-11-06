---
type: concept
system: "[[Serialization]]"
source: "Source/serial/NumberText.hpp"
---

`std::to_chars` everywhere in `serial` — never `printf` or `ostream`, which follow the locale. A test pins the output under a Turkish locale. `IniLoad` and `parseJson` parse with `std::from_chars`. `serial/NumberText` holds the shared formatting.

Exception still open: `TextLoad` parses scene numbers with `std::stod`/`stoll`, which follow the locale and accept numeric prefixes. Tracked in `TODO.md`.
