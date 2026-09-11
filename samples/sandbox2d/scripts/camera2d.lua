return {
    speed = 200,

    start = function(self)
        engine.log("wasd/arrows pan, scroll zooms, click spawns")
    end,

    update = function(self, dt)
        if engine.keyPressed("escape") then engine.quit() end

        local actor = self.actor
        local camera = actor.Camera
        local move = vec3(0, 0, 0)

        if engine.keyDown("a") or engine.keyDown("left")  then move.x = move.x - 1 end
        if engine.keyDown("d") or engine.keyDown("right") then move.x = move.x + 1 end
        if engine.keyDown("w") or engine.keyDown("up")    then move.y = move.y - 1 end
        if engine.keyDown("s") or engine.keyDown("down")  then move.y = move.y + 1 end

        actor.position = actor.position + move * (self.speed * dt / camera.zoom)

        local _, scroll = engine.scroll()
        if scroll ~= 0 then
            camera.zoom = camera.zoom * (1 + scroll * 0.1)
        end
    end,
}
