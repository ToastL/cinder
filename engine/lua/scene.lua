local nodeMt = {}
local cache = setmetatable({}, { __mode = "v" })
local signals = {}

function __node(id)
    if id == nil then return nil end
    local node = cache[id]
    if node == nil then
        node = setmetatable({ id = id }, nodeMt)
        cache[id] = node
    end
    return node
end

local function signalOf(id, key)
    local byKey = signals[id]
    if byKey == nil then
        byKey = {}
        signals[id] = byKey
    end
    local found = byKey[key]
    if found == nil then
        found = signal()
        byKey[key] = found
    end
    return found
end

local function fire(id, key, ...)
    local byKey = signals[id]
    local found = byKey and byKey[key]
    if found then found:fire(...) end
end

function __attributeChanged(id, name)
    fire(id, "attribute:" .. name)
    fire(id, "attributeChanged", name)
end

function __childAdded(parent, child)
    fire(parent, "childAdded", __node(child))
end

function __childRemoved(parent, child)
    fire(parent, "childRemoved", __node(child))
end

function __touched(a, b)
    fire(a, "touched", __node(b))
    fire(b, "touched", __node(a))
end

function __touchEnded(a, b)
    fire(a, "touchEnded", __node(b))
    fire(b, "touchEnded", __node(a))
end

function __destroying(id)
    fire(id, "destroying")
    signals[id] = nil
end

local function vector(x, y, z, w)
    if y == nil then return x end
    if z == nil then return vec2(x, y) end
    if w == nil then return vec3(x, y, z) end
    return vec4(x, y, z, w)
end

local function vec3Of(x, y, z)
    if x == nil then return nil end
    return vec3(x, y, z)
end

local function attributeValue(value)
    if type(value) ~= "table" then return value end
    return vector(value[1], value[2], value[3], value[4])
end

local function nodes(list)
    local out = {}
    for i, id in ipairs(list) do out[i] = __node(id) end
    return out
end

local get = {
    name             = function(self) return engine.name(self.id) end,
    className        = function(self) return engine.className(self.id) end,
    parent           = function(self) return __node(engine.parent(self.id)) end,
    position         = function(self) return vec3Of(engine.position(self.id)) end,
    rotation         = function(self) return vec3Of(engine.rotation(self.id)) end,
    scale            = function(self) return vec3Of(engine.scale(self.id)) end,
    worldPosition    = function(self) return vec3Of(engine.worldPosition(self.id)) end,
    forward          = function(self) return vec3Of(engine.forward(self.id)) end,
    right            = function(self) return vec3Of(engine.right(self.id)) end,
    up               = function(self) return vec3Of(engine.up(self.id)) end,
    childAdded       = function(self) return signalOf(self.id, "childAdded") end,
    childRemoved     = function(self) return signalOf(self.id, "childRemoved") end,
    destroying       = function(self) return signalOf(self.id, "destroying") end,
    attributeChanged = function(self) return signalOf(self.id, "attributeChanged") end,
    touched          = function(self) return signalOf(self.id, "touched") end,
    touchEnded       = function(self) return signalOf(self.id, "touchEnded") end,
}

local function spatial(self, key, ok)
    if not ok then error(tostring(self) .. " has no " .. key, 4) end
end

local set = {
    name     = function(self, v) engine.setName(self.id, v) end,
    parent   = function(self, v) engine.setParent(self.id, v and v.id or nil) end,
    position = function(self, v) spatial(self, "position", engine.setPosition(self.id, v:unpack())) end,
    rotation = function(self, v) spatial(self, "rotation", engine.setRotation(self.id, v:unpack())) end,
    scale    = function(self, v) spatial(self, "scale", engine.setScale(self.id, v:unpack())) end,
}

local fns = {}

function fns:getChildren()
    return nodes(engine.children(self.id))
end

function fns:findFirstChild(name)
    return __node(engine.findChild(self.id, name))
end

function fns:add(className)
    return __node(engine.create(className, self.id))
end

function fns:clone()
    return __node(engine.clone(self.id))
end

function fns:destroy()
    engine.destroy(self.id)
end

function fns:valid()
    return engine.valid(self.id)
end

function fns:translate(v)
    if not engine.translate(self.id, v:unpack()) then
        error(tostring(self) .. " has no position", 2)
    end
end

function fns:applyImpulse(impulse, point)
    if point then
        engine.applyImpulse(self.id, impulse.x, impulse.y, impulse.z, point.x, point.y, point.z)
    else
        engine.applyImpulse(self.id, impulse.x, impulse.y, impulse.z)
    end
end

function fns:applyForce(force, point)
    if point then
        engine.applyForce(self.id, force.x, force.y, force.z, point.x, point.y, point.z)
    else
        engine.applyForce(self.id, force.x, force.y, force.z)
    end
end

function fns:applyTorque(torque)
    engine.applyTorque(self.id, torque.x, torque.y, torque.z)
end

function fns:getAttribute(name)
    return attributeValue(engine.getAttribute(self.id, name))
end

function fns:setAttribute(name, value)
    engine.setAttribute(self.id, name, value)
end

function fns:getAttributes()
    local out = engine.getAttributes(self.id)
    for name, value in pairs(out) do out[name] = attributeValue(value) end
    return out
end

function fns:getAttributeChangedSignal(name)
    return signalOf(self.id, "attribute:" .. name)
end

nodeMt.__index = function(self, key)
    local getter = get[key]
    if getter then return getter(self) end

    local fn = fns[key]
    if fn then return fn end

    local x, y, z, w = engine.getProp(self.id, key)
    if x ~= nil then return vector(x, y, z, w) end

    return __node(engine.findChild(self.id, key))
end

nodeMt.__newindex = function(self, key, value)
    local setter = set[key]
    if setter then return setter(self, value) end

    local ok
    if vecSize(value) then
        ok = engine.setProp(self.id, key, value:unpack())
    else
        ok = engine.setProp(self.id, key, value)
    end
    if not ok then
        error(tostring(self) .. " has no property '" .. tostring(key) .. "'", 2)
    end
end

nodeMt.__tostring = function(self)
    return (engine.className(self.id) or "Node") .. " " .. (engine.name(self.id) or "?")
        .. "(" .. self.id .. ")"
end

local castRay = engine.raycast
local readGravity = engine.gravity
local writeGravity = engine.setGravity

function engine.raycast(origin, direction, distance)
    local id, px, py, pz, nx, ny, nz, reach =
        castRay(origin.x, origin.y, origin.z, direction.x, direction.y, direction.z, distance)
    if id == nil then return nil end
    return {
        node = __node(id),
        position = vec3(px, py, pz),
        normal = vec3(nx, ny, nz),
        distance = reach,
    }
end

function engine.gravity()
    return vec3(readGravity())
end

function engine.setGravity(gravity)
    writeGravity(gravity.x, gravity.y, gravity.z)
end

scene = {}

function scene:create(className, parent)
    return __node(engine.create(className, parent and parent.id or nil))
end

function scene:find(name)
    return __node(engine.find(name))
end

function scene:getChildren()
    return nodes(engine.roots())
end
