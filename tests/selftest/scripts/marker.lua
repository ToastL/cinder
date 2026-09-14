local marked = script.parent

_G.selftestMarks = _G.selftestMarks or {}
_G.selftestMarks[marked.name] = marked:getAttribute("mark")
leaked = true

task.wait(0.05)
_G.selftestYielded = true
