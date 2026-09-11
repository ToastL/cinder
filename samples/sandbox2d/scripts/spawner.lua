return {
    script = "scripts/riser.lua",
    minSize = 4,
    maxSize = 18,
    lifetime = 6,

    update = function(self, dt)
        if not engine.mouseDown("left") then return end

        local mx, my = engine.mousePosition()
        local wx, wy = engine.screenToWorld(mx, my)
        local size = self.minSize + math.random() * (self.maxSize - self.minSize)

        local box = scene:spawn("Box")
        box.position = vec3(wx, wy, 0)

        local sprite = box:add("SpriteRenderer")
        sprite.size = vec2(size, size)
        sprite.color = rgba(math.random(), math.random(), math.random(), 0.85)

        box:behaviour(self.script, { speed = 20 + math.random() * 80 })

        task.delay(self.lifetime, function()
            if box:valid() then box:destroy() end
        end)
    end,
}
