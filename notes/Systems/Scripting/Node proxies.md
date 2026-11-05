---
type: concept
system: "[[Scripting]]"
source: "engine/lua/scene.lua, src/script/SceneApi.cpp"
---

**Pure Lua over `engine.*` bindings that take node ids.** One proxy type for every node, cached weakly so `scene:find(n) == node`.

Reading a key looks, in order, at:
1. fixed getters — `name`, `className`, `parent`, `position`, `rotation`, `scale`, `worldPosition`, `forward`, `right`, `up`, and signals `childAdded`, `childRemoved`, `destroying`, `attributeChanged`;
2. methods — `getChildren`, `findFirstChild`, `add(className)`, `clone`, `destroy`, `valid`, `translate`, attribute methods;
3. the node's props via `engine.getProp` (1–4 values by arity → scalar or vector);
4. a child with that name.

A prop wins over a same-named child. Writing a key that's neither a setter nor a prop **errors**; so does writing a transform on a non-spatial node.

**Contracts that must not drift:**
- `getProp` returns 1–4 values by arity, **zero** when node or prop is missing; `setProp` returns whether the prop exists.
- `find` / `parent` / `findFirstChild` return **`nil`**, never `0` or `-1`.
- Node ids and texture/mesh handles push as **integers**; numeric props as **floats**; string and enum props as strings.
- Edge input clears per fixed step. `screenToWorld` takes window points and returns the point on `z = 0`.

`script/LuaProps` converts Lua ↔ `PropValue`: scalars, arrays, string-keyed records, vectors (metatable `__vec`).
