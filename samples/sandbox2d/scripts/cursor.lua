return {
    update = function(self, dt)
        local mx, my = engine.mousePosition()
        local wx, wy = engine.screenToWorld(mx, my)
        self.actor.position = vec3(wx, wy, 0)
    end,
}
