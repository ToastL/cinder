#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cinder::Dev {

enum class AssetKind : std::uint8_t { Folder, Scene, Prefab, Texture, Other };

struct AssetEntry {
    std::uint64_t id = 0;
    std::uint64_t parent = 0;
    std::string path;
    std::string name;
    AssetKind kind = AssetKind::Other;
};

class AssetCatalog {
public:
    static constexpr std::chrono::milliseconds POLL_INTERVAL{1000};
    static constexpr std::string_view NEW_FOLDER = "New Folder";

    static std::uint64_t idOf(std::string_view path);
    static AssetKind kindOf(const std::filesystem::path& path);
    static std::string_view label(AssetKind kind);
    static std::filesystem::path root();
    static std::vector<std::string> scan(std::initializer_list<AssetKind> kinds);

    static constexpr std::uint64_t ROOT = 0xcbf29ce484222325ull;

    void refresh();
    bool poll();
    std::uint64_t revision() const { return revision_; }

    const AssetEntry* byId(std::uint64_t id) const;
    const AssetEntry* byPath(std::string_view path) const;
    std::vector<std::uint64_t> subfolders(std::uint64_t folder) const;
    std::vector<std::uint64_t> contents(std::uint64_t folder) const;
    std::vector<std::uint64_t> search(std::string_view filter) const;
    std::vector<std::string> list(AssetKind kind) const;
    std::size_t size() const { return entries_.size(); }

    std::optional<std::string> createFolder(std::string_view parent, std::string_view name = NEW_FOLDER);
    bool remove(std::string_view path, const std::filesystem::path& keep, std::string* error = nullptr);

private:
    std::vector<AssetEntry> entries_;
    std::unordered_map<std::uint64_t, std::size_t> index_;
    std::uint64_t revision_ = 0;
    std::optional<std::chrono::steady_clock::time_point> scanned_;
};

}
