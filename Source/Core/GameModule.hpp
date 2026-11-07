#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::Scene { class ClassRegistry; }

namespace cinder::Core {

class Registrar;

const char* buildId();

class GameModule {
public:
    static std::string_view abi();
    static std::string_view extension();
    static std::string_view platform();
    static std::optional<std::filesystem::path> locate(const std::filesystem::path& project, std::string_view name);
    static std::unique_ptr<GameModule> open(const std::filesystem::path& library, cinder::Scene::ClassRegistry& types,
                                            std::string& error);

    ~GameModule();

    GameModule(const GameModule&) = delete;
    GameModule& operator=(const GameModule&) = delete;

    void enlist();
    const std::filesystem::path& path() const { return path_; }
    const std::vector<std::string>& classes() const { return classes_; }

private:
    using RegisterFn = void (*)(Registrar&);

    GameModule(void* handle, RegisterFn enlist, std::filesystem::path path, cinder::Scene::ClassRegistry& types);

    void* handle_;
    RegisterFn register_;
    std::filesystem::path path_;
    cinder::Scene::ClassRegistry& types_;
    std::vector<std::string> classes_;
};

}
