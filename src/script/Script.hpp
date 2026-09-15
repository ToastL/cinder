#pragma once

#include "scene/Node.hpp"

#include <lua.hpp>

#include <string>

namespace cinder::script {

class Script final : public cinder::scene::Node, public cinder::reflect::PropSink {
public:
    static const char* BOOTSTRAP;

    explicit Script(lua_State* state);
    Script(lua_State* state, std::string file);
    ~Script() override;

    void onStart() override;
    void onDestroy() override;
    void propChanged(const cinder::reflect::PropDef& prop) override;

    const std::string& file() const { return file_; }
    bool running() const { return owner_ != LUA_NOREF; }
    void reload();

    CINDER_NODE(Script, cinder::scene::Node) { CINDER_PROP(file_); }

private:
    void start();
    void stop();

    lua_State* state_;
    std::string file_;
    int owner_ = LUA_NOREF;
};

}
