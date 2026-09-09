#pragma once

#include <filesystem>
#include <string_view>

namespace cinder::platform {

void setAssetRoot(const std::filesystem::path& root);
const std::filesystem::path& assetRoot();
std::filesystem::path assetPath(std::string_view relative);
std::filesystem::path resolveAsset(std::string_view path);

}
