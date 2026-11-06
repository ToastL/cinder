#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cbt {

struct LayerGraph {
    std::set<std::string> modules;
    std::map<std::string, std::set<std::string>> allowed;
};

LayerGraph engineLayers(const std::filesystem::path& sourceRoot);
std::vector<std::string> checkLayers(const std::filesystem::path& sourceRoot, const LayerGraph& graph);

}
