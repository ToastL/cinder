#pragma once

#include "script/ScriptHost.hpp"

#include <lua.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace cinder::gfx { class Renderer; }
namespace cinder::platform { class Input; }
namespace cinder::scene { class Actor; class Scene; }

namespace cinder::script {

class LuaHost : public ScriptHost {
public:
    LuaHost(std::filesystem::path source, cinder::scene::Scene& scene, cinder::platform::Input& input,
            cinder::gfx::Renderer& renderer, std::function<void()> quit);
    ~LuaHost() override;

    LuaHost(const LuaHost&) = delete;
    LuaHost& operator=(const LuaHost&) = delete;

    void load() override;
    void poll() override;
    void update(float dt) override;
    void render(float alpha) override;

    void eval(const std::string& source);

private:
    struct Watch {
        std::filesystem::path file;
        std::int64_t modified;
    };

    static int behaviourRead(lua_State* state);

    void boot();
    void runEntry();
    void registerScripts();
    void registerApi();
    void loadPrelude();
    void watch(const char* path);
    void reloadBehaviour(const std::string& path);
    int reloadIn(cinder::scene::Actor& actor, const std::string& path);
    void close();

    std::filesystem::path source_;
    cinder::scene::Scene& scene_;
    cinder::platform::Input& input_;
    cinder::gfx::Renderer& renderer_;
    std::function<void()> quit_;

    lua_State* state_ = nullptr;
    std::int64_t lastModified_ = 0;
    std::unordered_map<std::string, Watch> watched_;
};

}
