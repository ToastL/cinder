local actor = script.actor

_G.selftestMarks = _G.selftestMarks or {}
_G.selftestMarks[actor.name] = actor:getAttribute("mark")
leaked = true

task.wait(0.05)
_G.selftestYielded = true
