return {
    start = function(self)
        local fails = 0
        local function check(what, ok)
            if not ok then fails = fails + 1 end
            engine.log((ok and "ok   " or "FAIL ") .. what)
        end
        local function near(a, b) return math.abs(a - b) < 1e-4 end

        check("vec add",        vec3(1, 2, 3) + vec3(1, 1, 1) == vec3(2, 3, 4))
        check("vec sub",        vec3(5, 5, 5) - vec3(1, 2, 3) == vec3(4, 3, 2))
        check("vec scalar mul", vec3(1, 2, 3) * 2 == vec3(2, 4, 6))
        check("scalar vec mul", 2 * vec3(1, 2, 3) == vec3(2, 4, 6))
        check("vec negate",     -vec3(1, -2, 3) == vec3(-1, 2, -3))
        check("vec length",     near(vec3(3, 4, 0):length(), 5))
        check("vec normalized", near(vec3(0, 7, 0):normalized().y, 1))
        check("vec dot",        near(vec3(1, 2, 3):dot(vec3(4, 5, 6)), 32))
        check("vec cross",      vec3(1, 0, 0):cross(vec3(0, 1, 0)) == vec3(0, 0, 1))
        check("vec tostring",   tostring(vec2(1.5, 2)) == "(1.5, 2)")
        check("vec unpack",     select("#", vec4(1, 2, 3, 4):unpack()) == 4)
        check("vecSize",        vecSize(vec2(0, 0)) == 2 and vecSize(7) == nil)

        local c = rgba(0.1, 0.2, 0.3, 0.4)
        check("color channels", near(c.r, 0.1) and near(c.g, 0.2) and near(c.b, 0.3) and near(c.a, 0.4))
        c.r = 0.9
        check("color write",    near(c.x, 0.9))

        local a = scene:spawn("Alpha")
        check("spawn",          a ~= nil and a.name == "Alpha")
        a.name = "Renamed"
        check("name write",     a.name == "Renamed")
        check("actor identity", scene:find("Renamed") == a)

        a.position = vec3(1, 2, 3)
        check("position rt",    a.position == vec3(1, 2, 3))
        a.position = a.position + vec3(0, 1, 0)
        check("position math",  a.position == vec3(1, 3, 3))
        a.scale = vec3(2, 2, 2)
        check("scale rt",       a.scale == vec3(2, 2, 2))

        local kid = a:spawn("Kid")
        check("child spawn",    kid.parent == a)
        check("children list",  #a:children() == 1)
        check("child by name",  a:child("Kid") == kid)
        kid.parent = nil
        check("reparent nil",   kid.parent == nil)

        check("active default", a.active == true)
        a.active = false
        check("active write",   a.active == false)
        a.active = true

        local sprite = a:add("SpriteRenderer")
        check("add component",  sprite ~= nil)
        check("get component",  a:get("SpriteRenderer") ~= nil)
        check("dot component",  a.SpriteRenderer ~= nil)
        check("missing comp",   a:get("Spin") == nil)

        sprite.size = vec2(12, 34)
        check("vec2 prop rt",   sprite.size == vec2(12, 34))
        sprite.color = rgba(0.5, 0.25, 0.125, 1)
        check("vec4 prop rt",   near(sprite.color.r, 0.5) and near(sprite.color.b, 0.125))
        sprite.texture = "none.png"
        check("string prop rt", sprite.texture == "none.png")
        sprite.texture = ""
        check("bool prop rt",   sprite.enabled == true)
        sprite.enabled = false
        check("bool prop write", sprite.enabled == false)
        sprite.enabled = true

        local cam = a:add("Camera")
        cam.projection = "orthographic"
        check("enum prop rt",   cam.projection == "orthographic")
        cam.fov = 500
        check("clamp via proxy", near(cam.fov, 179))

        local ok = pcall(function() a.nonsense = 1 end)
        check("unknown prop errors", not ok)

        local ticks = 0
        stepped:connect(function(dt) ticks = ticks + 1 end)

        local waited = false
        task.spawn(function()
            task.wait(0.1)
            waited = true
        end)

        local delayed = false
        task.delay(0.15, function() delayed = true end)

        scene:spawn("Spawned"):behaviour("scripts/marker.lua", { mark = 7 })

        task.delay(0.4, function()
            local marks = _G.selftestMarks or {}
            check("stepped signal", ticks > 5)
            check("task.wait", waited)
            check("task.delay", delayed)
            check("behaviour data from scene", marks.Behaved == 42)
            check("behaviour data override", marks.Spawned == 7)
            check("behaviour coroutine start", _G.selftestYielded == true)
            engine.log(fails == 0 and "ALL PASS" or (fails .. " FAILED"))
            engine.quit()
        end)
    end,
}
