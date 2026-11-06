---
type: module
system: "[[Serialization]]"
source: "Source/serial/Archive.hpp"
---

`Archive` is the bidirectional visitor that `SceneCodec::walk` and `ProjectConfig::walk` run in both directions.

- **Text** (`TextLoad`/`TextSave`) — scenes. See [[Scene file format]].
- **INI** (`IniLoad`/`IniSave`) — a record is a `[Section]`, a field is `key=value` with camelCase keys, a value runs raw to end of line. No arrays, attribute bags or nested sections: asking for one throws `std::logic_error`. `IniSave` writes only non-defaults and drops empty sections. See [[Game.ini]].
- **JSON** (`serial/Json`) — `parseJson` builds a `PropValue` (objects `PropRec`, arrays `PropSeq`, `null` rejected), errors as `line:column`; `JsonWriter` streams so the caller decides key order. See [[Project descriptor]].
