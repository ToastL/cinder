#include "Dev/AssetCatalog.hpp"

#include "Platform/Assets.hpp"

#include <algorithm>
#include <exception>
#include <system_error>

namespace cinder::Dev {
namespace {

char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

std::string lowered(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), lower);
    return out;
}

bool contains(std::string_view text, std::string_view needle) {
    return lowered(text).find(lowered(needle)) != std::string::npos;
}

bool hidden(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    return name.empty() || name.front() == '.';
}

bool inside(const std::filesystem::path& path, const std::filesystem::path& folder) {
    const std::filesystem::path relative = path.lexically_normal().lexically_relative(folder.lexically_normal());
    return !relative.empty() && *relative.begin() != "..";
}

std::vector<AssetEntry> walk() {
    std::vector<AssetEntry> entries;
    AssetEntry top;
    top.id = AssetCatalog::ROOT;
    top.parent = AssetCatalog::ROOT;
    top.name = "Content";
    top.kind = AssetKind::Folder;
    entries.push_back(top);

    const std::filesystem::path root = AssetCatalog::root();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error)) return entries;

    std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied,
                                                     error);
    for (const std::filesystem::recursive_directory_iterator end; !error && it != end; it.increment(error)) {
        const std::filesystem::path& path = it->path();
        if (hidden(path)) {
            if (it->is_directory(error)) it.disable_recursion_pending();
            continue;
        }
        const bool folder = it->is_directory(error);
        if (!folder && !it->is_regular_file(error)) continue;
        AssetEntry entry;
        entry.path = path.lexically_relative(root).generic_string();
        const std::string parent = std::filesystem::path(entry.path).parent_path().generic_string();
        entry.id = AssetCatalog::idOf(entry.path);
        entry.parent = AssetCatalog::idOf(parent);
        entry.name = path.filename().string();
        entry.kind = folder ? AssetKind::Folder : AssetCatalog::kindOf(path);
        entries.push_back(std::move(entry));
    }
    std::sort(entries.begin() + 1, entries.end(),
              [](const AssetEntry& a, const AssetEntry& b) { return a.path < b.path; });
    return entries;
}

}

std::uint64_t AssetCatalog::idOf(std::string_view path) {
    std::uint64_t hash = ROOT;
    for (const char c : path) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ull;
    }
    return hash;
}

AssetKind AssetCatalog::kindOf(const std::filesystem::path& path) {
    const std::string extension = lowered(path.extension().string());
    if (extension == ".scene") return AssetKind::Scene;
    if (extension == ".prefab") return AssetKind::Prefab;
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga"
        || extension == ".bmp" || extension == ".gif") {
        return AssetKind::Texture;
    }
    return AssetKind::Other;
}

std::string_view AssetCatalog::label(AssetKind kind) {
    switch (kind) {
        case AssetKind::Folder: return "Folder";
        case AssetKind::Scene: return "Scene";
        case AssetKind::Prefab: return "Prefab";
        case AssetKind::Texture: return "Texture";
        case AssetKind::Other: return "File";
    }
    return "File";
}

std::filesystem::path AssetCatalog::root() { return cinder::Platform::projectRoot() / "Content"; }

std::vector<std::string> AssetCatalog::scan(std::initializer_list<AssetKind> kinds) {
    std::vector<std::string> files;
    for (const AssetEntry& entry : walk()) {
        if (std::find(kinds.begin(), kinds.end(), entry.kind) != kinds.end() && entry.id != ROOT) {
            files.push_back(entry.path);
        }
    }
    return files;
}

void AssetCatalog::refresh() {
    std::vector<AssetEntry> next = walk();
    scanned_ = std::chrono::steady_clock::now();
    const bool same = next.size() == entries_.size()
                      && std::equal(next.begin(), next.end(), entries_.begin(), [](const AssetEntry& a, const AssetEntry& b) {
                             return a.path == b.path && a.kind == b.kind;
                         });
    if (same) return;
    entries_ = std::move(next);
    index_.clear();
    for (std::size_t i = 0; i < entries_.size(); ++i) index_.emplace(entries_[i].id, i);
    ++revision_;
}

bool AssetCatalog::poll() {
    const auto now = std::chrono::steady_clock::now();
    if (scanned_ && now - *scanned_ < POLL_INTERVAL) return false;
    const std::uint64_t before = revision_;
    refresh();
    return revision_ != before;
}

const AssetEntry* AssetCatalog::byId(std::uint64_t id) const {
    const auto found = index_.find(id);
    return found == index_.end() ? nullptr : &entries_[found->second];
}

const AssetEntry* AssetCatalog::byPath(std::string_view path) const { return byId(idOf(path)); }

std::vector<std::uint64_t> AssetCatalog::subfolders(std::uint64_t folder) const {
    std::vector<std::uint64_t> out;
    for (const AssetEntry& entry : entries_) {
        if (entry.id != ROOT && entry.parent == folder && entry.kind == AssetKind::Folder) out.push_back(entry.id);
    }
    return out;
}

std::vector<std::uint64_t> AssetCatalog::contents(std::uint64_t folder) const {
    std::vector<std::uint64_t> folders = subfolders(folder);
    for (const AssetEntry& entry : entries_) {
        if (entry.id != ROOT && entry.parent == folder && entry.kind != AssetKind::Folder) folders.push_back(entry.id);
    }
    return folders;
}

std::vector<std::uint64_t> AssetCatalog::search(std::string_view filter) const {
    std::vector<std::uint64_t> out;
    for (const AssetEntry& entry : entries_) {
        if (entry.id != ROOT && entry.kind != AssetKind::Folder && contains(entry.name, filter)) out.push_back(entry.id);
    }
    return out;
}

std::vector<std::string> AssetCatalog::list(AssetKind kind) const {
    std::vector<std::string> out;
    for (const AssetEntry& entry : entries_) {
        if (entry.id != ROOT && entry.kind == kind) out.push_back(entry.path);
    }
    return out;
}

std::optional<std::string> AssetCatalog::createFolder(std::string_view parent, std::string_view name) {
    std::filesystem::path base;
    try {
        base = parent.empty() ? root() : cinder::Platform::contentPath(parent);
    } catch (const std::exception&) {
        return std::nullopt;
    }
    std::error_code error;
    if (!std::filesystem::is_directory(base, error)) return std::nullopt;
    for (int n = 1; n < 1000; ++n) {
        const std::string candidate = n == 1 ? std::string(name) : std::string(name) + " " + std::to_string(n);
        const std::filesystem::path path = base / candidate;
        if (std::filesystem::exists(path, error)) continue;
        if (!std::filesystem::create_directory(path, error) || error) return std::nullopt;
        refresh();
        return path.lexically_relative(root()).generic_string();
    }
    return std::nullopt;
}

bool AssetCatalog::remove(std::string_view path, const std::filesystem::path& keep, std::string* error) {
    const auto fail = [error](std::string message) {
        if (error != nullptr) *error = std::move(message);
        return false;
    };
    if (path.empty()) return fail("Content itself cannot be deleted");
    std::filesystem::path target;
    try {
        target = cinder::Platform::contentPath(path);
    } catch (const std::exception&) {
        return fail("Only files inside Content/ can be deleted");
    }
    if (!keep.empty() && (target.lexically_normal() == keep.lexically_normal() || inside(keep, target))) {
        return fail("The open scene cannot be deleted");
    }
    std::error_code code;
    if (!std::filesystem::exists(target, code)) return fail(std::string(path) + " does not exist");
    std::filesystem::remove_all(target, code);
    if (code) return fail(code.message());
    refresh();
    return true;
}

}
