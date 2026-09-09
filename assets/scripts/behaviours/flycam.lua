return {
    yaw = 0,
    pitch = -0.17,
    sensitivity = 0.0021,
    speed = 8,
    sprint = 24,

    update = function(self, dt)
        local actor = self.actor

        if engine.mousePressed("left") and not engine.cursorLocked() then
            engine.setCursorLocked(true)
        end
        if engine.keyPressed("escape") then
            if engine.cursorLocked() then
                engine.setCursorLocked(false)
            else
                engine.quit()
            end
        end

        if engine.cursorLocked() then
            local dx, dy = engine.mouseDelta()
            self.yaw = self.yaw - dx * self.sensitivity
            self.pitch = math.max(-1.55, math.min(1.55, self.pitch - dy * self.sensitivity))
            actor.rotation = vec3(self.pitch, self.yaw, 0)
        end

        local move = vec3(0, 0, 0)
        if engine.keyDown("w") then move = move + actor.forward end
        if engine.keyDown("s") then move = move - actor.forward end
        if engine.keyDown("d") then move = move + actor.right end
        if engine.keyDown("a") then move = move - actor.right end
        if engine.keyDown("space") then move.y = move.y + 1 end
        if engine.keyDown("lctrl") then move.y = move.y - 1 end

        local speed = engine.keyDown("lshift") and self.sprint or self.speed
        actor:translate(move * (speed * dt))
    end,
}
