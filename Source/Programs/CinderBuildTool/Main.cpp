#include "CinderBuildTool/Build.hpp"
#include "CinderBuildTool/Layers.hpp"
#include "CinderBuildTool/Generate.hpp"
#include "CinderBuildTool/Registry.hpp"
#include "CinderBuildTool/Tests.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace {

const std::filesystem::path ROOT = CBT_ROOT;

std::filesystem::path executable() {
#if defined(__APPLE__)
    char buffer[4096];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) return std::filesystem::canonical(buffer);
#else
    std::error_code error;
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) return self;
#endif
    return {};
}

bool isRuleFile(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    return name.ends_with(".Build.cpp") || name.ends_with(".Target.cpp");
}

std::set<std::filesystem::path> ruleFilesOnDisk() {
    std::set<std::filesystem::path> out;
    for (const char* folder : {"Source", "Tests"}) {
        std::error_code error;
        for (auto it = std::filesystem::recursive_directory_iterator(ROOT / folder, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            if (it->is_regular_file() && isRuleFile(it->path())) out.insert(std::filesystem::weakly_canonical(it->path()));
        }
    }
    return out;
}

bool stale(const std::filesystem::path& self) {
    std::error_code error;
    const auto built = std::filesystem::last_write_time(self, error);
    if (error) return false;

    std::set<std::filesystem::path> compiled;
    for (const cbt::ModuleEntry& entry : cbt::registeredModules()) compiled.insert(std::filesystem::weakly_canonical(entry.file));
    for (const cbt::TargetEntry& entry : cbt::registeredTargets()) compiled.insert(std::filesystem::weakly_canonical(entry.file));
    const std::set<std::filesystem::path> onDisk = ruleFilesOnDisk();
    if (compiled != onDisk) return true;

    for (const std::filesystem::path& file : onDisk) {
        if (std::filesystem::last_write_time(file, error) > built) return true;
    }
    for (const auto& entry : std::filesystem::directory_iterator(ROOT / "Source" / "Programs" / "CinderBuildTool", error)) {
        if (entry.is_regular_file() && entry.last_write_time() > built) return true;
    }
    return std::filesystem::last_write_time(ROOT / "Setup.sh", error) > built;
}

void rebuildIfStale(char** argv) {
    if (std::getenv("CBT_REBUILT") != nullptr) return;
    const std::filesystem::path self = executable();
    if (self.empty() || !stale(self)) return;

    std::fprintf(stderr, "cbt: rules or tool sources changed, rebuilding cbt\n");
    const std::string binaries = self.parent_path().parent_path().string();
    setenv("CINDER_BINARIES", binaries.c_str(), 1);
    const std::string command = "/bin/bash '" + (ROOT / "Setup.sh").string() + "'";
    if (std::system(command.c_str()) != 0) {
        std::fprintf(stderr, "cbt: rebuilding cbt failed\n");
        std::exit(1);
    }
    setenv("CBT_REBUILT", "1", 1);
    execv(self.c_str(), argv);
    std::perror("cbt: exec");
    std::exit(1);
}

int usage() {
    std::fprintf(stderr,
                 "usage: cbt <command>\n"
                 "  check [--source dir]   check first-party includes against the module rules\n"
                 "  modules                list every module and its dependencies\n"
                 "  build [targets...] [--config Debug|Development|Shipping] [--backend mtl|vk]\n"
                 "        [--shader-formats spirv,msl,dxil] [--output dir] [--path dirs] [-j n]\n"
                 "  test [layers|layer_fixtures|shaders|shipping|unit|hot_reload...] [build options]\n"
                 "  game <project> [--library file] [build options]   build a project's C++\n"
                 "  generate xcode|compile-commands [targets] [build options]\n"
                 "  clean [--config c] [--output dir]\n");
    return 2;
}

int check(const std::vector<std::string_view>& args) {
    std::filesystem::path source = ROOT / "Source";
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--source" && i + 1 < args.size()) source = args[++i];
        else return usage();
    }
    const cbt::LayerGraph graph = cbt::engineLayers(ROOT / "Source");
    const std::vector<std::string> errors = cbt::checkLayers(source, graph);
    for (const std::string& error : errors) std::fprintf(stderr, "%s\n", error.c_str());
    if (errors.empty()) std::printf("layers ok: %zu modules\n", graph.modules.size());
    return errors.empty() ? 0 : 1;
}

bool parseOptions(const std::vector<std::string_view>& args, cbt::BuildOptions& options, std::vector<std::string>& names) {
    options.root = ROOT;
    options.output = ROOT;
    options.self = executable();
    for (std::size_t i = 0; i < args.size(); ++i) {
        const bool value = i + 1 < args.size();
        if (args[i] == "--config" && value) options.config = std::string(args[++i]);
        else if (args[i] == "--backend" && value) options.backend = std::string(args[++i]);
        else if (args[i] == "--path" && value) setenv("PATH", std::string(args[++i]).c_str(), 1);
        else if (args[i] == "--output" && value) options.output = std::filesystem::absolute(args[++i]);
        else if (args[i] == "-j" && value) options.jobs = std::atoi(std::string(args[++i]).c_str());
        else if (args[i] == "--shader-formats" && value) {
            const std::string list(args[++i]);
            for (std::size_t start = 0; start <= list.size();) {
                const std::size_t comma = std::min(list.find(',', start), list.size());
                if (comma > start) options.shaderFormats.push_back(list.substr(start, comma - start));
                start = comma + 1;
            }
        } else if (args[i].starts_with("-")) return false;
        else names.emplace_back(args[i]);
    }
    if (options.config != "Debug" && options.config != "Development" && options.config != "Shipping") return false;
    return options.backend.empty() || options.backend == "mtl" || options.backend == "vk";
}

int game(const std::vector<std::string_view>& args) {
    cbt::BuildOptions options;
    std::vector<std::string> names;
    std::vector<std::string_view> rest;
    std::filesystem::path library;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--library" && i + 1 < args.size()) library = std::filesystem::absolute(args[++i]);
        else rest.push_back(args[i]);
    }
    if (!parseOptions(rest, options, names) || names.size() != 1) return usage();
    try {
        cbt::Builder builder(cbt::Toolchain::detect(), options);
        return builder.buildGame(std::filesystem::absolute(names.front()), library) ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "cbt: %s\n", e.what());
        return 1;
    }
}

const std::vector<std::string> EDITOR_TARGETS{"editor", "player", "packager", "ui_gallery", "tests", "graphics_smoke"};

int generate(const std::vector<std::string_view>& args) {
    if (args.empty()) return usage();
    cbt::BuildOptions options;
    std::vector<std::string> names;
    if (!parseOptions({args.begin() + 1, args.end()}, options, names)) return usage();
    try {
        cbt::Builder builder(cbt::Toolchain::detect(), options);
        if (args[0] == "compile-commands") {
            return cbt::generateCompileCommands(builder, names.empty() ? std::vector<std::string>{"editor", "ui_gallery", "tests", "graphics_smoke"} : names) ? 0 : 1;
        }
        if (args[0] == "xcode") {
            const std::filesystem::path copy = builder.binaries().parent_path() / "cbt";
            builder.build({});
            return cbt::generateXcode(builder, names.empty() ? EDITOR_TARGETS : names, copy) ? 0 : 1;
        }
        return usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "cbt: %s\n", e.what());
        return 1;
    }
}

int xcode(const std::vector<std::string_view>& args) {
    cbt::BuildOptions options;
    std::vector<std::string> names;
    if (!parseOptions(args, options, names) || names.size() < 2) return usage();
    options.config = names[1];
    if (options.config != "Debug" && options.config != "Development" && options.config != "Shipping") return usage();
    try {
        cbt::Builder builder(cbt::Toolchain::detect(), options);
        if (names.size() > 2 && names[2] == "clean") return builder.clean() ? 0 : 1;
        return builder.build({names[0]}) ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "cbt: %s\n", e.what());
        return 1;
    }
}

int clean(const std::vector<std::string_view>& args) {
    cbt::BuildOptions options;
    std::vector<std::string> names;
    if (!parseOptions(args, options, names) || !names.empty()) return usage();
    try {
        cbt::Builder builder(cbt::Toolchain::detect(), options);
        return builder.clean() ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "cbt: %s\n", e.what());
        return 1;
    }
}

int build(const std::vector<std::string_view>& args, bool test) {
    cbt::BuildOptions options;
    std::vector<std::string> names;
    if (!parseOptions(args, options, names)) return usage();
    try {
        cbt::Builder builder(cbt::Toolchain::detect(), options);
        if (test) return cbt::runTests(builder, names);
        if (names.empty()) names = {"editor", "ui_gallery", "tests"};
        return builder.build(names) ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "cbt: %s\n", e.what());
        return 1;
    }
}

int listModules() {
    const cbt::LayerGraph graph = cbt::engineLayers(ROOT / "Source");
    for (const cbt::ModuleEntry& entry : cbt::registeredModules()) {
        std::printf("%s  (%s)\n", entry.name.c_str(), entry.file.lexically_relative(ROOT).c_str());
        auto found = graph.allowed.find(entry.name);
        if (found == graph.allowed.end()) continue;
        std::string line;
        for (const std::string& name : found->second) line += " " + name;
        std::printf("    may include:%s\n", line.c_str());
    }
    for (const cbt::TargetEntry& entry : cbt::registeredTargets()) {
        std::printf("target %s  (%s)\n", entry.name.c_str(), entry.file.lexically_relative(ROOT).c_str());
    }
    return 0;
}

}

int main(int argc, char** argv) {
    rebuildIfStale(argv);
    if (argc < 2) return usage();
    const std::string_view command = argv[1];
    const std::vector<std::string_view> args(argv + 2, argv + argc);
    if (command == "check") return check(args);
    if (command == "modules") return listModules();
    if (command == "build") return build(args, false);
    if (command == "test") return build(args, true);
    if (command == "game") return game(args);
    if (command == "generate") return generate(args);
    if (command == "xcode") return xcode(args);
    if (command == "clean") return clean(args);
    return usage();
}
