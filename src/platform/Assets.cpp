#include "platform/Assets.hpp"

#include <cstdlib>

#ifndef CINDER_ASSETS_DEFAULT
#define CINDER_ASSETS_DEFAULT "assets"
#endif

namespace cinder::platform {
namespace {

std::filesystem::path root;
bool resolved = false;

std::filesystem::path resolve() {
    if (const char* env = std::getenv("CINDER_ASSETS"); env != nullptr && *env != '\0') return env;

    const std::filesystem::path baked(CINDER_ASSETS_DEFAULT);
    if (std::filesystem::exists(baked)) return baked;

    return std::filesystem::path("assets");
}

}

void setAssetRoot(const std::filesystem::path& value) {
    root = value;
    resolved = true;
}

const std::filesystem::path& assetRoot() {
    if (!resolved) {
        root = resolve();
        resolved = true;
    }
    return root;
}

std::filesystem::path assetPath(std::string_view relative) {
    return assetRoot() / std::filesystem::path(relative);
}

std::filesystem::path resolveAsset(std::string_view path) {
    std::string_view trimmed = path;
    if (trimmed.starts_with("./")) trimmed.remove_prefix(2);
    if (trimmed.starts_with("assets/")) trimmed.remove_prefix(7);
    return assetRoot() / std::filesystem::path(trimmed);
}

}
