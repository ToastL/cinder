---
type: concept
system: "[[Reflection]]"
source: "Source/reflect/Reflect.hpp"
---

```cpp
CINDER_NODE(Camera, cinder::scene::Spatial) {
    CINDER_PROP(projection_);
    CINDER_PROP_S(fov_, 1.0f, 179.0f, 1.0f);
    CINDER_PROP_R(zoom_, 0.05f, 20.0f);
}
```

- `CINDER_PROP_R` adds a clamp range, `CINDER_PROP_S` a range and a drag step, `CINDER_PROP_COLOR` marks a vec3/vec4 as a colour, `CINDER_PROP_ANGLE` marks radians shown in degrees. Only [[Properties panel]] reads step and hint; colours and angles are never clamped.
- `props<T>()` builds the list once into a function-local static. `PropChain` emits base-class props first, so `Node::enabled` is always the first prop — the delta serializer depends on that order.
- `PropBuilder` is templated on the **concrete** type, so inherited props assume no base-subobject offset.

**Four behaviours, each pinned by a test:**
1. **Vector writes mutate in place** — `Transform::position()` hands out a live reference.
2. **Clamping is write-only** — a save never clamps live values.
3. **Enums serialize as their lowercase name**; an unknown constant is silently rejected with no write and no notify. `CINDER_ENUM_NAMES` declares the table; comparison is ASCII-only.
4. **`PropSink::propChanged`** fires after every successful write. `Transform` uses it to `dirty()`; renderers use it to drop cached handles ([[Texture and mesh names]]).
