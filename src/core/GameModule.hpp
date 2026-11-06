#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::scene { class NodeTypes; }

namespace cinder::core {

class GameModule {
public:
    static std::string_view abi();
    static std::string_view extension();
    static std::string_view platform();
    static std::optional<std::filesystem::path> locate(const std::filesystem::path& project, std::string_view name);
    static std::unique_ptr<GameModule> open(const std::filesystem::path& library, cinder::scene::NodeTypes& types,
                                            std::string& error);

    ~GameModule();

    GameModule(const GameModule&) = delete;
    GameModule& operator=(const GameModule&) = delete;

    void enlist();
    const std::filesystem::path& path() const { return path_; }
    const std::vector<std::string>& classes() const { return classes_; }

private:
    GameModule(void* handle, std::filesystem::path path, cinder::scene::NodeTypes& types);

    void* handle_;
    std::filesystem::path path_;
    cinder::scene::NodeTypes& types_;
    std::vector<std::string> classes_;
};

}
