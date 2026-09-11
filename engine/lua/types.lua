local function vectype(keys)
    local n = #keys
    local mt = {}
    local new

    mt.__vec = n
    mt.__index = mt

    new = function(...)
        local a = { ... }
        local t = {}
        for i = 1, n do t[keys[i]] = a[i] or 0 end
        return setmetatable(t, mt)
    end

    local function combine(a, b, f)
        local t = {}
        if type(b) == "number" then
            for i = 1, n do t[keys[i]] = f(a[keys[i]], b) end
        elseif type(a) == "number" then
            for i = 1, n do t[keys[i]] = f(a, b[keys[i]]) end
        else
            for i = 1, n do t[keys[i]] = f(a[keys[i]], b[keys[i]]) end
        end
        return setmetatable(t, mt)
    end

    mt.__add = function(a, b) return combine(a, b, function(x, y) return x + y end) end
    mt.__sub = function(a, b) return combine(a, b, function(x, y) return x - y end) end
    mt.__mul = function(a, b) return combine(a, b, function(x, y) return x * y end) end
    mt.__div = function(a, b) return combine(a, b, function(x, y) return x / y end) end
    mt.__unm = function(a) return combine(a, -1, function(x, y) return x * y end) end

    mt.__eq = function(a, b)
        for i = 1, n do
            if a[keys[i]] ~= b[keys[i]] then return false end
        end
        return true
    end

    mt.__tostring = function(a)
        local parts = {}
        for i = 1, n do parts[i] = string.format("%.4g", a[keys[i]]) end
        return "(" .. table.concat(parts, ", ") .. ")"
    end

    function mt:unpack()
        local out = {}
        for i = 1, n do out[i] = self[keys[i]] end
        return table.unpack(out, 1, n)
    end

    function mt:length()
        local sum = 0
        for i = 1, n do sum = sum + self[keys[i]] * self[keys[i]] end
        return math.sqrt(sum)
    end

    function mt:dot(other)
        local sum = 0
        for i = 1, n do sum = sum + self[keys[i]] * other[keys[i]] end
        return sum
    end

    function mt:normalized()
        local len = self:length()
        if len == 0 then return new() end
        return self / len
    end

    return mt, new
end

local vec2Mt, vec2New = vectype({ "x", "y" })
local vec3Mt, vec3New = vectype({ "x", "y", "z" })
local vec4Mt, vec4New = vectype({ "x", "y", "z", "w" })

vec2 = vec2New
vec3 = vec3New
vec4 = vec4New
rgba = vec4New

function vec3Mt:cross(other)
    return vec3(self.y * other.z - self.z * other.y,
                self.z * other.x - self.x * other.z,
                self.x * other.y - self.y * other.x)
end

local channel = { r = "x", g = "y", b = "z", a = "w" }

vec4Mt.__index = function(t, k)
    local mapped = channel[k]
    if mapped then return rawget(t, mapped) end
    return vec4Mt[k]
end

vec4Mt.__newindex = function(t, k, v)
    rawset(t, channel[k] or k, v)
end

function vecSize(v)
    if type(v) ~= "table" then return nil end
    local mt = getmetatable(v)
    return mt and mt.__vec
end
