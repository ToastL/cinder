#include "CinderBuildTool/Layers.hpp"

#include "CinderBuildTool/Registry.hpp"

#include <fstream>
#include <regex>

namespace cbt {
namespace {

std::string ownerOf(const std::string& path, const std::set<std::string>& modules) {
    std::string best;
    for (const std::string& module : modules) {
        if (path.size() > module.size() && path.compare(0, module.size(), module) == 0 && path[module.size()] == '/'
            && module.size() > best.size()) {
            best = module;
        }
    }
    return best;
}

void closure(const std::string& name, const std::map<std::string, std::set<std::string>>& publics,
             std::set<std::string>& out) {
    auto found = publics.find(name);
    if (found == publics.end()) return;
    for (const std::string& dependency : found->second) {
        if (out.insert(dependency).second) closure(dependency, publics, out);
    }
}

bool outside(const std::string& relative) {
    return relative == "ThirdParty" || relative == "Programs" || relative.starts_with("ThirdParty/")
           || relative.starts_with("Programs/");
}

}

LayerGraph engineLayers(const std::filesystem::path& sourceRoot) {
    LayerGraph graph;
    std::map<std::string, std::set<std::string>> publics;
    std::map<std::string, std::set<std::string>> direct;
    const std::filesystem::path root = std::filesystem::weakly_canonical(sourceRoot);

    for (const ModuleEntry& entry : registeredModules()) {
        const std::filesystem::path directory = std::filesystem::weakly_canonical(entry.file.parent_path());
        const std::filesystem::path relative = directory.lexically_relative(root);
        if (relative.empty() || relative.native().starts_with("..") || outside(relative.generic_string())) continue;
        graph.modules.insert(entry.name);
        for (const char* backend : {"mtl", "vk"}) {
            const ModuleRules rules = evaluate(entry, TargetInfo{"check", "", "Development", backend, "", "", root, root, root});
            for (const std::string& name : rules.publicDependencies_) {
                publics[entry.name].insert(name);
                direct[entry.name].insert(name);
            }
            for (const std::string& name : rules.privateDependencies_) direct[entry.name].insert(name);
        }
    }

    for (const std::string& module : graph.modules) {
        std::set<std::string>& allowed = graph.allowed[module];
        for (const std::string& dependency : direct[module]) {
            allowed.insert(dependency);
            closure(dependency, publics, allowed);
        }
    }
    return graph;
}

std::vector<std::string> checkLayers(const std::filesystem::path& sourceRoot, const LayerGraph& graph) {
    static const std::regex INCLUDE(R"(^[ \t]*#[ \t]*include[ \t]+["<]([^">]+)[">])");
    std::vector<std::string> errors;
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(sourceRoot, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        if (it->is_directory() && outside(it->path().lexically_relative(sourceRoot).generic_string())) {
            it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file()) continue;
        const std::string extension = it->path().extension().string();
        if (extension != ".cpp" && extension != ".hpp" && extension != ".mm") continue;

        const std::string source = it->path().lexically_relative(sourceRoot).generic_string();
        const std::string layer = ownerOf(source, graph.modules);
        if (layer.empty()) {
            errors.push_back("Unknown source layer: " + source);
            continue;
        }
        const std::set<std::string>& allowed = graph.allowed.at(layer);

        std::ifstream in(it->path());
        std::string line;
        while (std::getline(in, line)) {
            std::smatch match;
            if (!std::regex_search(line, match, INCLUDE)) continue;
            const std::string header = match[1].str();
            if (header.starts_with("./") || header.starts_with("../") || header.find("/./") != std::string::npos
                || header.find("/../") != std::string::npos) {
                errors.push_back("Include must use the src root: " + source + " -> " + header);
                continue;
            }
            const std::string dependency = ownerOf(header, graph.modules);
            if (dependency.empty() || dependency == layer) continue;
            if (allowed.count(dependency) == 0) {
                errors.push_back("Forbidden layer dependency: " + source + " -> " + header);
            }
        }
    }
    return errors;
}

}
