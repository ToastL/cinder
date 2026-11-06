---
type: format
system: "[[Serialization]]"
source: "Source/serial/SceneCodec.cpp"
---

`SceneCodec` writes version 4 (`VERSION` and `OLDEST` are both 4; the old actor/component formats aren't read).

```
version 4
nodes {
    MeshPart {
        id 2
        name "Box"
        position -6 0 -6
        children {
            Spinner {
                id 3
                speed 0 0.5 0
            }
        }
    }
}
```

A block's name is its class. A node writes `id`, `name` (omitted when it equals the class), its transform if spatial, class props that differ from the class default, `attributes`, then `children`. An unknown class is logged and skipped with its subtree; an attribute value it can't hold is logged and dropped.

Ids round-trip, which is what lets [[Selection]]-by-id survive Play, Stop and undo.
