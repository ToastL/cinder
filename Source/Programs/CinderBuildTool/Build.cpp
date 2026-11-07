#include "CinderBuildTool/Build.hpp"

#include "CinderBuildTool/Paths.hpp"
#include "CinderBuildTool/Process.hpp"
#include "CinderBuildTool/Registry.hpp"

#include <algorithm>
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace cbt {
namespace {

bool isSource(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    return extension == ".cpp" || extension == ".cc" || extension == ".c" || extension == ".m" || extension == ".mm";
}

bool within(const std::filesystem::path& path, const std::filesystem::path& folder) {
    const std::string a = path.generic_string();
    const std::string b = folder.generic_string();
    return a.size() > b.size() && a.compare(0, b.size(), b) == 0 && a[b.size()] == '/';
}

std::string flat(const std::string& name) {
    std::string out = name;
    std::replace(out.begin(), out.end(), '/', '_');
    return out;
}

std::string inQuotes(const std::string& value) { return "\"" + value + "\""; }

std::string jsonString(std::string_view value) {
    std::string out = "\"";
    for (const char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char escaped[8];
                    std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(c));
                    out += escaped;
                } else {
                    out.push_back(c);
                }
        }
    }
    return out + "\"";
}

bool writeIfChanged(const std::filesystem::path& path, const std::string& text) {
    std::ifstream in(path);
    std::stringstream current;
    current << in.rdbuf();
    if (in && current.str() == text) return true;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << text;
    return static_cast<bool>(out);
}

}

Builder::Builder(Toolchain toolchain, BuildOptions options) : toolchain_(std::move(toolchain)), options_(std::move(options)) {
    if (options_.backend.empty()) options_.backend = toolchain_.platform == "macOS" ? "mtl" : "vk";
    if (options_.shaderFormats.empty()) {
        options_.shaderFormats = toolchain_.platform == "macOS" ? std::vector<std::string>{"spirv", "msl"}
                                                                : std::vector<std::string>{"spirv"};
    }
    if (options_.jobs <= 0) options_.jobs = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
}

const std::string& Builder::buildId() const {
    if (!buildId_) buildId_ = computeBuildId();
    return *buildId_;
}

std::filesystem::path Builder::binaries() const {
    return options_.output / "Binaries" / toolchain_.platform / options_.config;
}

std::filesystem::path Builder::intermediate() const {
    return options_.output / "Intermediate" / toolchain_.platform / options_.config;
}

TargetInfo Builder::info(const std::string& target) const { return info(target, buildId()); }

TargetInfo Builder::info(const std::string& target, const std::string& id) const {
    return TargetInfo{target, toolchain_.platform, options_.config, options_.backend, ENGINE_VERSION, id,
                      options_.root, binaries(), intermediate()};
}

std::string Builder::computeBuildId() const {
    std::uint64_t hash = 1469598103934665603ull;
    const auto mix = [&hash](std::string_view data) {
        for (const char c : data) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211ull;
        }
        hash ^= 0xff;
        hash *= 1099511628211ull;
    };
    mix(ENGINE_VERSION);
    mix(options_.backend);
    mix(options_.config);
    mix(toolchain_.identity);

    std::vector<std::filesystem::path> files;
    const std::filesystem::path source = options_.root / "Source";
    const std::filesystem::path outsideAbi[] = {source / "dev", source / "editor", source / "packager",
                                                source / "Programs"};
    {
        std::error_code error;
        for (auto it = std::filesystem::recursive_directory_iterator(source, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            if (it->is_directory() && std::find(std::begin(outsideAbi), std::end(outsideAbi), it->path()) != std::end(outsideAbi)) {
                it.disable_recursion_pending();
                continue;
            }
            const std::string extension = it->path().extension().string();
            if (it->is_regular_file() && (isSource(it->path()) || extension == ".hpp" || extension == ".h"
                                          || extension == ".hh" || extension == ".inl")) {
                files.push_back(it->path());
            }
        }
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        mix(file.lexically_relative(options_.root).generic_string());
        std::ifstream in(file, std::ios::binary);
        std::stringstream content;
        content << in.rdbuf();
        mix(content.str());
    }

    char text[17];
    std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
    return text;
}

std::vector<std::string> Builder::configFlags() const {
    if (options_.config == "Debug") return {"-O0", "-g"};
    if (options_.config == "Shipping") return {"-O2", "-DNDEBUG"};
    return {"-O2", "-g"};
}

const Builder::Module& Builder::module(const std::string& name, const TargetInfo& info) {
    auto found = modules_.find(name);
    if (found != modules_.end()) return found->second;
    const ModuleEntry* entry = findModule(name);
    if (entry == nullptr) throw std::runtime_error("unknown module " + name);
    Module module{evaluate(*entry, info), false};
    module.firstParty = !module.rules.thirdParty_;
    return modules_.emplace(name, std::move(module)).first->second;
}

void Builder::publicClosure(const std::string& name, const TargetInfo& info, std::set<std::string>& out) {
    if (!out.insert(name).second) return;
    for (const std::string& dependency : module(name, info).rules.publicDependencies_) publicClosure(dependency, info, out);
}

std::vector<std::string> Builder::closure(const std::vector<std::string>& roots, const TargetInfo& info) {
    std::vector<std::string> order;
    std::set<std::string> seen;
    std::vector<std::string> stack(roots.rbegin(), roots.rend());
    while (!stack.empty()) {
        const std::string name = stack.back();
        stack.pop_back();
        if (!seen.insert(name).second) continue;
        order.push_back(name);
        const ModuleRules& rules = module(name, info).rules;
        for (const auto* list : {&rules.publicDependencies_, &rules.privateDependencies_, &rules.linkDependencies_}) {
            for (const std::string& dependency : *list) stack.push_back(dependency);
        }
    }
    return order;
}

std::vector<std::filesystem::path> Builder::sourcesOf(const Module& module) const {
    const ModuleRules& rules = module.rules;
    if (rules.headerOnly_) return {};
    if (rules.explicitSources_) return rules.sources_;

    std::vector<std::filesystem::path> nested;
    for (const ModuleEntry& entry : registeredModules()) {
        if (within(entry.file.parent_path(), rules.directory())) nested.push_back(entry.file.parent_path());
    }
    std::vector<std::filesystem::path> out;
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(rules.directory(), error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        const std::filesystem::path path = it->path();
        if (it->is_directory()) {
            const bool skip = std::any_of(nested.begin(), nested.end(), [&](const auto& folder) { return path == folder; })
                    || std::any_of(rules.excluded_.begin(), rules.excluded_.end(), [&](const auto& folder) { return path == folder; });
            if (skip) it.disable_recursion_pending();
            continue;
        }
        if (isSource(path) && !isRuleFile(path)) out.push_back(path);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string> Builder::compileFlags(const std::string& name, const TargetInfo& info, bool positionIndependent,
                                               const std::vector<std::string>& extraDefines) {
    const Module& self = module(name, info);
    std::vector<std::string> flags = configFlags();
    flags.insert(flags.end(), toolchain_.sysroot.begin(), toolchain_.sysroot.end());
    if (positionIndependent) flags.insert(flags.end(), {"-fPIC", "-fvisibility=hidden"});
    if (self.firstParty) {
        flags.insert(flags.end(), {"-Wall", "-Wextra", "-Wno-unused-parameter"});
        flags.push_back("-I" + (options_.root / "Source").string());
        flags.push_back("-I" + (intermediate() / "Generated").string());
    } else {
        flags.push_back("-w");
    }
    for (const auto& include : self.rules.privateIncludes_) flags.push_back("-I" + include.string());
    for (const auto& include : self.rules.publicIncludes_) flags.push_back("-I" + include.string());
    for (const std::string& define : self.rules.privateDefines_) flags.push_back("-D" + define);
    for (const std::string& define : self.rules.publicDefines_) flags.push_back("-D" + define);
    for (const std::string& define : extraDefines) flags.push_back("-D" + define);

    std::set<std::string> visible;
    for (const auto* list : {&self.rules.publicDependencies_, &self.rules.privateDependencies_}) {
        for (const std::string& dependency : *list) publicClosure(dependency, info, visible);
    }
    for (const std::string& dependency : visible) {
        const Module& other = module(dependency, info);
        for (const auto& include : other.rules.publicIncludes_) {
            flags.push_back(other.firstParty ? "-I" + include.string() : "-isystem");
            if (!other.firstParty) flags.push_back(include.string());
        }
        for (const std::string& define : other.rules.publicDefines_) flags.push_back("-D" + define);
    }
    flags.insert(flags.end(), self.rules.compileFlags_.begin(), self.rules.compileFlags_.end());
    return flags;
}

void Builder::planShaders(std::vector<Action>& actions, std::vector<std::filesystem::path>& outputs) {
    const std::filesystem::path out = binaries() / "Shaders";
    const std::string dxc = findTool("dxc", "DXC");
    if (dxc.empty()) throw std::runtime_error("dxc not found; install the LunarG Vulkan SDK, put dxc on PATH or set DXC");
    const auto has = [&](const char* format) {
        return std::find(options_.shaderFormats.begin(), options_.shaderFormats.end(), format) != options_.shaderFormats.end();
    };
    std::string spirvCross;
    bool metal = false;
    if (has("msl")) {
        spirvCross = findTool("spirv-cross", "SPIRV_CROSS");
        if (spirvCross.empty()) throw std::runtime_error("spirv-cross not found, but msl shaders were asked for; put it on PATH or set SPIRV_CROSS");
        std::string error;
        std::unique_ptr<Process> probe = Process::start({"xcrun", "metal", "--version"}, error);
        if (probe != nullptr) {
            std::string line;
            while (probe->readLine(line)) {}
            metal = probe->wait() == 0;
        }
        if (!metal) std::printf("cbt: Metal Toolchain unavailable; emitting .metal source only\n");
    }

    std::vector<std::filesystem::path> programs;
    for (const auto& entry : std::filesystem::directory_iterator(options_.root / "Shaders")) {
        if (entry.path().extension() == ".hlsl") programs.push_back(entry.path());
    }
    std::sort(programs.begin(), programs.end());
    for (const std::filesystem::path& source : programs) {
        const std::string name = source.stem().string();
        std::vector<std::filesystem::path> airs;
        for (const std::string stage : {"vs", "ps"}) {
            const std::string entry = stage == "vs" ? "VSMain" : "PSMain";
            const std::filesystem::path spv = out / (name + "." + stage + ".spv");
            actions.push_back(Action{"dxc spirv " + name + "." + stage,
                                     {dxc, "-spirv", "-fspv-target-env=vulkan1.2", "-T", stage + "_6_0", "-E", entry,
                                      source.string(), "-Fo", spv.string()},
                                     {}, {source}, {spv}, {}, {}});
            outputs.push_back(spv);
            if (has("dxil")) {
                const std::filesystem::path dxil = out / (name + "." + stage + ".dxil");
                actions.push_back(Action{"dxc dxil " + name + "." + stage,
                                         {dxc, "-T", stage + "_6_0", "-E", entry, source.string(), "-Fo", dxil.string()},
                                         {}, {source}, {dxil}, {}, {}});
                outputs.push_back(dxil);
            }
            if (!has("msl")) continue;
            const std::filesystem::path msl = out / (name + "." + stage + ".metal");
            std::vector<std::string> cross{spirvCross, "--msl", "--msl-version", "20300", "--msl-decoration-binding"};
            if (stage == "vs") cross.push_back("--flip-vert-y");
            cross.insert(cross.end(), {"--rename-entry-point", entry, entry, stage == "vs" ? "vert" : "frag", spv.string(),
                                       "--output", msl.string()});
            actions.push_back(Action{"spirv-cross msl " + name + "." + stage, cross, {}, {spv}, {msl}, {}, {}});
            const std::filesystem::path reflect = out / (name + "." + stage + ".json");
            actions.push_back(Action{"spirv-cross reflect " + name + "." + stage,
                                     {spirvCross, "--reflect", "--output", reflect.string(), spv.string()},
                                     {}, {spv}, {reflect}, {}, {}});
            outputs.insert(outputs.end(), {msl, reflect});
            if (!metal) continue;
            const std::filesystem::path air = out / (name + "." + stage + ".air");
            actions.push_back(Action{"metal " + name + "." + stage,
                                     {"xcrun", "metal", "-std=macos-metal2.3",
                                      "-fmodules-cache-path=" + (intermediate() / "MetalModuleCache").string(), "-c", msl.string(),
                                      "-o", air.string()},
                                     {}, {msl}, {air}, {}, {}});
            airs.push_back(air);
        }
        if (!airs.empty()) {
            const std::filesystem::path library = out / (name + ".metallib");
            std::vector<std::string> command{"xcrun", "metallib"};
            for (const auto& air : airs) command.push_back(air.string());
            command.insert(command.end(), {"-o", library.string()});
            actions.push_back(Action{"metallib " + name, command, {}, airs, {library}, {}, {}});
            outputs.push_back(library);
        }
    }
}

void Builder::planTarget(const std::string& name, std::set<std::string>& planned, std::vector<Action>& actions) {
    if (!planned.insert(name).second) return;
    const TargetEntry* entry = findTarget(name);
    if (entry == nullptr) throw std::runtime_error("unknown target " + name);
    const TargetInfo targetInfo = info(name);
    const TargetRules target = evaluate(*entry, targetInfo);
    for (const std::string& prerequisite : target.prerequisites_) planTarget(prerequisite, planned, actions);
    planRules(name, target, targetInfo, actions, Placement{});
}

void Builder::planRules(const std::string& name, const TargetRules& target, const TargetInfo& targetInfo,
                        std::vector<Action>& actions, const Placement& placement) {
    const bool game = target.type_ == TargetType::GameModule;
    const std::vector<std::string> modules = game ? target.modules_ : closure(target.modules_, targetInfo);
    const std::set<std::string> owned(target.modules_.begin(), target.modules_.end());

    std::vector<std::string> defines = target.defines_;
    if (game && std::none_of(defines.begin(), defines.end(), [](const std::string& d) { return d.starts_with("CINDER_MODULE_ABI="); })) {
        defines.push_back("CINDER_MODULE_ABI=" + inQuotes(placement.id.empty() ? buildId() : placement.id));
    }

    std::vector<std::filesystem::path> objects;
    std::set<std::string> frameworks;
    std::set<std::string> libraries;
    for (const std::string& moduleName : modules) {
        const Module& current = module(moduleName, targetInfo);
        for (const auto& framework : current.rules.frameworks_) frameworks.insert(framework);
        for (const auto& library : current.rules.systemLibraries_) libraries.insert(library);
        const bool mine = owned.count(moduleName) != 0;
        const std::filesystem::path base = !placement.objects.empty() ? placement.objects / flat(moduleName)
                : mine ? intermediate() / "Targets" / name / flat(moduleName)
                       : intermediate() / "Modules" / flat(moduleName);
        const std::vector<std::string> flags = compileFlags(moduleName, targetInfo, game, mine ? defines : std::vector<std::string>{});

        for (const std::filesystem::path& source : sourcesOf(current)) {
            const std::filesystem::path relative = source.lexically_relative(current.rules.directory());
            const std::filesystem::path object = base / (relative.generic_string() + ".o");
            const std::filesystem::path depfile = base / (relative.generic_string() + ".d");
            const std::string extension = source.extension().string();
            const bool c = extension == ".c" || extension == ".m";
            std::vector<std::string> command{c ? toolchain_.cc : toolchain_.cxx};
            if (!c) command.push_back("-std=c++20");
            command.insert(command.end(), flags.begin(), flags.end());
            if (extension == ".mm") command.insert(command.end(), current.rules.objcFlags_.begin(), current.rules.objcFlags_.end());
            command.insert(command.end(), {"-MMD", "-MF", depfile.string(), "-c", source.string(), "-o", object.string()});
            actions.push_back(Action{"compile " + source.lexically_relative(options_.root).generic_string(), command, {},
                                     {source}, {object}, depfile, {}});
            objects.push_back(object);
        }
        if (mine && !placement.entry.empty() && moduleName == target.modules_.front()) {
            const std::filesystem::path object = base / "ModuleEntry.cpp.o";
            std::vector<std::string> command{toolchain_.cxx, "-std=c++20"};
            command.insert(command.end(), flags.begin(), flags.end());
            command.insert(command.end(), {"-c", placement.entry.string(), "-o", object.string()});
            actions.push_back(Action{"compile ModuleEntry.cpp", command, {}, {placement.entry}, {object}, {}, {}});
            objects.push_back(object);
        }
    }

    if (!game && std::find(modules.begin(), modules.end(), "core") != modules.end()) {
        const std::filesystem::path source = intermediate() / "Generated" / "BuildId.cpp";
        const std::filesystem::path object = intermediate() / "Modules" / "core" / "BuildId.cpp.o";
        std::vector<std::string> command{toolchain_.cxx, "-std=c++20"};
        const std::vector<std::string> flags = compileFlags("core", targetInfo, false, {});
        command.insert(command.end(), flags.begin(), flags.end());
        command.insert(command.end(), {"-c", source.string(), "-o", object.string()});
        actions.push_back(Action{"compile BuildId.cpp", command, {}, {source}, {object}, {}, {}});
        objects.push_back(object);
    }

    std::vector<std::filesystem::path> linkInputs = objects;
    if (!game && std::find(modules.begin(), modules.end(), "gfx") != modules.end() && shaderPlanned_.insert("shaders").second) {
        std::vector<std::filesystem::path> shaderOutputs;
        planShaders(actions, shaderOutputs);
    }

    const std::string output = target.outputName_.empty() ? name : target.outputName_;
    const std::filesystem::path product = !placement.product.empty() ? placement.product
            : binaries() / (game ? output + toolchain_.libraryExtension() : output);
    std::vector<std::string> link{toolchain_.cxx};
    link.insert(link.end(), toolchain_.sysroot.begin(), toolchain_.sysroot.end());
    if (game) {
        link.push_back("-shared");
        if (toolchain_.platform == "macOS") link.insert(link.end(), {"-undefined", "dynamic_lookup"});
    }
    for (const auto& object : objects) link.push_back(object.string());
    for (const auto& framework : frameworks) link.insert(link.end(), {"-framework", framework});
    for (const auto& library : libraries) link.push_back("-l" + library);
    if (target.exportSymbols_) link.push_back(toolchain_.platform == "macOS" ? "-Wl,-export_dynamic" : "-rdynamic");
    if (toolchain_.platform == "Linux" && !game) link.insert(link.end(), {"-pthread", "-ldl"});
    link.insert(link.end(), {"-o", product.string()});
    actions.push_back(Action{"link " + product.filename().string(), link, {}, linkInputs, {product}, {}, {}});
}

bool writeCompileCommands(const std::filesystem::path& file, const std::filesystem::path& directory,
                          const std::vector<Action>& actions) {
    std::string commands = "[";
    for (const Action& action : actions) {
        if (!action.description.starts_with("compile ")) continue;
        const auto found = std::find(action.command.begin(), action.command.end(), "-c");
        if (found == action.command.end() || found + 1 == action.command.end()) continue;
        std::string arguments;
        for (const std::string& arg : action.command) arguments += (arguments.empty() ? "" : ", ") + jsonString(arg);
        commands += std::string(commands.size() > 1 ? "," : "") + "\n\t{\"directory\": " + jsonString(directory.string())
                + ", \"file\": " + jsonString(*(found + 1)) + ", \"arguments\": [" + arguments + "]}";
    }
    return writeIfChanged(file, commands + "\n]\n");
}

bool Builder::clean() {
    std::error_code error;
    std::filesystem::remove_all(intermediate(), error);
    if (error) return false;
    for (const auto& entry : std::filesystem::directory_iterator(binaries(), error)) {
        if (entry.path().filename() != "Engine.modules") std::filesystem::remove_all(entry.path(), error);
    }
    std::filesystem::remove(binaries() / "Engine.modules", error);
    std::printf("cbt: cleaned %s and %s\n", intermediate().string().c_str(), binaries().string().c_str());
    return true;
}

bool Builder::buildGame(const std::filesystem::path& project, std::filesystem::path library) {
    std::vector<std::filesystem::path> descriptors;
    for (const auto& entry : std::filesystem::directory_iterator(project)) {
        if (entry.path().extension() == ".cinder" && entry.is_regular_file()) descriptors.push_back(entry.path());
    }
    if (descriptors.empty()) throw std::runtime_error(project.string() + " is not a project: it has no .cinder file");
    if (descriptors.size() > 1) {
        std::sort(descriptors.begin(), descriptors.end());
        std::string names;
        for (const std::filesystem::path& path : descriptors) names += (names.empty() ? "" : ", ") + path.filename().string();
        throw std::runtime_error(project.string() + " has more than one .cinder file: " + names);
    }
    const std::filesystem::path descriptor = descriptors.front();
    const std::string name = descriptor.stem().string();

    std::ifstream manifest(binaries() / "Engine.modules");
    std::stringstream buffer;
    buffer << manifest.rdbuf();
    const std::string text = buffer.str();
    const std::string key = "\"buildId\": \"";
    const std::size_t at = text.find(key);
    if (at == std::string::npos) {
        throw std::runtime_error("no engine build in " + binaries().string() + "; run cbt build first");
    }
    const std::string engineId = text.substr(at + key.size(), text.find('"', at + key.size()) - at - key.size());

    const std::filesystem::path source = project / "Source";
    const std::filesystem::path objects = project / "Saved" / "Intermediate" / toolchain_.platform / options_.config;
    std::vector<std::filesystem::path> rules;
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(source, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        if (it->is_regular_file() && isRuleFile(it->path())) rules.push_back(it->path());
    }
    std::vector<std::string> modules;
    if (rules.empty()) {
        registerModule("Project." + name, source / (name + ".Build.cpp"),
                       [](ModuleRules& m, const TargetInfo&) { m.publicDependencies({"game"}); });
        modules.push_back("Project." + name);
    } else {
        std::set<std::string> engine;
        for (const ModuleEntry& module : registeredModules()) engine.insert(module.name);
        const std::size_t before = registeredModules().size();
        const std::filesystem::path library = objects / ("Rules" + toolchain_.libraryExtension());
        std::filesystem::create_directories(objects);
        std::vector<std::string> command{toolchain_.cxx, "-std=c++20", "-fPIC", "-shared"};
        command.insert(command.end(), toolchain_.sysroot.begin(), toolchain_.sysroot.end());
        command.push_back("-I" + (options_.root / "Source" / "Programs").string());
        for (const auto& file : rules) command.push_back(file.string());
        if (toolchain_.platform == "macOS") command.insert(command.end(), {"-undefined", "dynamic_lookup"});
        command.insert(command.end(), {"-o", library.string()});
        std::string failure;
        std::unique_ptr<Process> process = Process::start(command, failure);
        if (process == nullptr) throw std::runtime_error(failure);
        std::string line;
        while (process->readLine(line)) std::printf("%s\n", line.c_str());
        if (process->wait() != 0) throw std::runtime_error("the project's build rules did not compile");
        if (dlopen(library.c_str(), RTLD_NOW | RTLD_GLOBAL) == nullptr) throw std::runtime_error(dlerror());
        for (std::size_t i = before; i < registeredModules().size(); ++i) {
            const std::string& module = registeredModules()[i].name;
            if (engine.count(module) != 0) throw std::runtime_error("the project's module " + module + " has the name of an engine module");
            modules.push_back(module);
        }
        if (modules.empty()) throw std::runtime_error("the project's build rules register no module");
    }

    const std::filesystem::path entry = objects / "ModuleEntry.cpp";
    if (!writeIfChanged(entry, "#include \"core/Module.hpp\"\nCINDER_MODULE_ENTRY()\n")) {
        throw std::runtime_error("could not write " + entry.string());
    }
    if (library.empty()) library = project / "Saved" / "Binaries" / (name + toolchain_.libraryExtension());

    TargetRules target(name, source);
    target.type(TargetType::GameModule);
    for (const std::string& module : modules) target.modules_.push_back(module);
    std::vector<Action> actions;
    planRules(name, target, info(name, engineId), actions, Placement{objects, library, entry, engineId});

    writeCompileCommands(project / "compile_commands.json", project, actions);

    Executor executor(objects / "Actions.txt", options_.jobs);
    const ExecutorResult result = executor.run(std::move(actions));
    std::printf("cbt: %s %s — %d ran, %d up to date (%s, %s)\n", name.c_str(), result.ok ? "built" : "FAILED", result.ran,
                result.upToDate, options_.config.c_str(), options_.backend.c_str());
    return result.ok;
}

std::vector<Action> Builder::plan(const std::vector<std::string>& targets) {
    const std::string backend = "#pragma once\n\n#include \"gfx/" + options_.backend + "/Backend.hpp\"\n";
    if (!writeIfChanged(intermediate() / "Generated" / "gfx" / "rhi" / "Backend.hpp", backend)) {
        throw std::runtime_error("could not write the generated Backend.hpp");
    }

    const std::string id = "namespace cinder::core {\n\nconst char* buildId() { return \"" + buildId() + "\"; }\n\n}\n";
    const std::string modules = "{\n\t\"buildId\": \"" + buildId() + "\",\n\t\"version\": \"" + ENGINE_VERSION
            + "\",\n\t\"config\": \"" + options_.config + "\",\n\t\"backend\": \"" + options_.backend + "\"\n}\n";
    if (!writeIfChanged(intermediate() / "Generated" / "BuildId.cpp", id) || !writeIfChanged(binaries() / "Engine.modules", modules)) {
        throw std::runtime_error("could not write the BuildId");
    }

    if (!options_.self.empty()) {
        const std::filesystem::path copy = binaries().parent_path() / "cbt";
        std::error_code error;
        if (!std::filesystem::equivalent(options_.self, copy, error)) {
            std::filesystem::create_directories(copy.parent_path());
            std::filesystem::copy_file(options_.self, copy, std::filesystem::copy_options::update_existing, error);
        }
    }

    std::vector<Action> actions;
    std::set<std::string> planned;
    for (const std::string& target : targets) planTarget(target, planned, actions);

    std::vector<Action> unique;
    std::set<std::string> seen;
    for (Action& action : actions) {
        const std::string key = action.outputs.empty() ? action.description : action.outputs.front().string();
        if (seen.insert(key).second) unique.push_back(std::move(action));
    }
    return unique;
}

bool Builder::build(const std::vector<std::string>& targets) {
    std::vector<Action> unique = plan(targets);
    Executor executor(intermediate() / "Actions.txt", options_.jobs);
    const ExecutorResult result = executor.run(std::move(unique));
    std::printf("cbt: %s — %d ran, %d up to date (%s, %s, %s)\n", result.ok ? "build succeeded" : "build FAILED",
                result.ran, result.upToDate, toolchain_.platform.c_str(), options_.config.c_str(), options_.backend.c_str());
    return result.ok;
}

}
