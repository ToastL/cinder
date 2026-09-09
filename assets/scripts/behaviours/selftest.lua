return {
    mark = 0,

    start = function(self)
        _G.selftestMark = self.mark
        _G.selftestActorName = self.actor.name
        task.wait(0.05)
        _G.selftestYielded = true
    end,
}
