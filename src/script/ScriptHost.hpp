#pragma once

namespace cinder::script {

class ScriptHost {
public:
    virtual ~ScriptHost() = default;

    virtual void boot() = 0;
    virtual void poll() = 0;
    virtual void update(float dt) = 0;
    virtual void render(float alpha) = 0;
};

}
