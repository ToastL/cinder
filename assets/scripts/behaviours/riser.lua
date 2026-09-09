return {
    speed = 40,

    update = function(self, dt)
        self.actor:translate(vec3(0, self.speed * dt, 0))
    end,
}
