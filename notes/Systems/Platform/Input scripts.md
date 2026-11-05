---
type: tool
system: "[[Platform]]"
source: "src/platform/InputScript.hpp"
---

`platform/InputScript` replays a text file of timed input through `Input::inject`, ignoring the OS:

```
12 move 40 60
13 click left
14 type Crate
15 tap enter
20 capture out.png
21 quit
```

Coordinates are window points. `editor` also accepts `close`, which exercises the unsaved-changes prompt. Passed with `--input-script <file>` to `editor` and `ui_gallery`. It's how a panel change is verified without a human; [[graphics_smoke]] drives the whole editor the same way. See [[Verification recipes]].
