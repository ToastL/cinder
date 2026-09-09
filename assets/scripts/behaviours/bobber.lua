return {
    phase = 0,
    rate = 2,

    update = function(self, dt)
        local h = 1 + math.sin(engine.time() * self.rate + self.phase)
        local p = self.actor.position
        self.actor.position = vec3(p.x, h * 0.5, p.z)
        self.actor.scale = vec3(1, h, 1)
    end,
}
