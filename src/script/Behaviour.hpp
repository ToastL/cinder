#pragma once

#include "scene/Component.hpp"

#include <lua.hpp>

#include <string>

namespace cinder::script {

class Behaviour final : public cinder::scene::Component {
public:
    static const char* BOOTSTRAP;

    explicit Behaviour(lua_State* state);
    Behaviour(lua_State* state, std::string script, int data);
    ~Behaviour() override;

    void onStart() override;
    void onUpdate(float dt) override;
    void onDestroy() override;

    const std::string& script() const { return script_; }
    void reload();

    CINDER_COMPONENT(Behaviour, cinder::scene::Component) { CINDER_PROP(script_); }

private:
    int instantiate(int data);
    int snapshot();
    void release();

    lua_State* state_;
    std::string script_;
    int data_ = LUA_NOREF;
    int ref_ = LUA_NOREF;
};

}
