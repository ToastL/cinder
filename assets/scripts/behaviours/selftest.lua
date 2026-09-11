return {
    mark = 0,

    start = function(self)
        _G.selftestMarks = _G.selftestMarks or {}
        _G.selftestMarks[self.actor.name] = self.mark
        task.wait(0.05)
        _G.selftestYielded = true
    end,
}
