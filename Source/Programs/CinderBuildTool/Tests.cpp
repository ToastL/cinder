#include "CinderBuildTool/Tests.hpp"

#include "CinderBuildTool/Layers.hpp"
#include "CinderBuildTool/Paths.hpp"
#include "CinderBuildTool/Process.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>

namespace cbt {
namespace {

struct Fixture {
    const char* name;
    const char* source;
    const char* content;
    bool passes;
};

const Fixture FIXTURES[] = {
    {"allowed", "core/Example.cpp", "#include \"scene/Scene.hpp\"\n#include <vector>\n", true},
    {"ui_up", "ui/core/Example.cpp", "#include \"scene/Scene.hpp\"\n", false},
    {"ui_down", "ui/widgets/Example.cpp", "#include \"ui/core/Widget.hpp\"\n#include \"text/Font.hpp\"\n", true},
    {"ui_sideways", "ui/core/Example.cpp", "#include \"ui/widgets/Button.hpp\"\n", false},
    {"ui_flat", "ui/Example.cpp", "#include <vector>\n", false},
    {"renderer_ui", "gfx/Example.cpp", "#include \"ui/core/ElementList.hpp\"\n", true},
    {"renderer_widgets", "gfx/Example.cpp", "#include \"ui/widgets/Button.hpp\"\n", false},
    {"text_leaf", "text/Example.cpp", "#include \"gfx/Renderer.hpp\"\n", false},
    {"upward", "scene/Example.cpp", "#include \"core/Engine.hpp\"\n", false},
    {"renderer_parent", "gfx/vk/Example.cpp", "#include \"gfx/Renderer.hpp\"\n", false},
    {"runtime_editor", "core/Example.cpp", "#include \"dev/History.hpp\"\n", false},
    {"relative", "scene/Example.cpp", "#include \"../core/Engine.hpp\"\n", false},
    {"rhi_is_a_leaf", "gfx/rhi/Example.cpp", "#include \"gfx/vk/VkCtx.hpp\"\n", false},
    {"rhi_no_scene", "gfx/rhi/Example.cpp", "#include \"scene/Scene.hpp\"\n", false},
    {"backend_on_rhi", "gfx/vk/Example.cpp", "#include \"gfx/rhi/Handles.hpp\"\n", true},
    {"pass_on_rhi", "gfx/pass/Example.cpp", "#include \"gfx/rhi/Commands.hpp\"\n", true},
    {"backend_leak_gfx", "gfx/Example.cpp", "#include \"gfx/vk/Ctx.hpp\"\n", false},
    {"backend_leak_pass", "gfx/pass/Example.cpp", "#include \"gfx/vk/Ctx.hpp\"\n", false},
    {"backend_leak_asset", "gfx/asset/Example.cpp", "#include \"gfx/vk/Ctx.hpp\"\n", false},
    {"backend_leak_core", "core/Example.cpp", "#include \"gfx/vk/Ctx.hpp\"\n", false},
    {"backends_are_blind", "gfx/mtl/Example.cpp", "#include \"gfx/vk/Ctx.hpp\"\n", false},
    {"metal_on_rhi", "gfx/mtl/Example.cpp", "#include \"gfx/rhi/Handles.hpp\"\n", true},
    {"unknown", "unknown/Example.cpp", "#include <vector>\n", false},
    {"metal_implementation_on_rhi", "gfx/mtl/Example.mm", "#include \"gfx/rhi/Handles.hpp\"\n", true},
    {"metal_implementation_upward", "gfx/mtl/Example.mm", "#include \"core/Engine.hpp\"\n", false},
};

bool run(const std::vector<std::string>& argv, std::string* captured = nullptr) {
    std::string error;
    std::unique_ptr<Process> process = Process::start(argv, error);
    if (process == nullptr) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return false;
    }
    std::string line;
    while (process->readLine(line)) {
        if (captured != nullptr) *captured += line + "\n";
        else std::printf("%s\n", line.c_str());
    }
    return process->wait() == 0;
}

bool layers(Builder& builder) {
    const std::vector<std::string> errors = checkLayers(builder.root() / "Source", engineLayers(builder.root() / "Source"));
    for (const std::string& error : errors) std::printf("%s\n", error.c_str());
    return errors.empty();
}

bool layerFixtures(Builder& builder) {
    const LayerGraph graph = engineLayers(builder.root() / "Source");
    bool ok = true;
    for (const Fixture& fixture : FIXTURES) {
        const std::filesystem::path root = builder.intermediate() / "LayerFixtures" / fixture.name;
        std::filesystem::remove_all(root);
        std::filesystem::create_directories((root / fixture.source).parent_path());
        std::ofstream(root / fixture.source) << fixture.content;
        const bool passed = checkLayers(root, graph).empty();
        if (passed != fixture.passes) {
            std::printf("fixture %s unexpectedly %s\n", fixture.name, passed ? "passed" : "failed");
            ok = false;
        }
    }
    return ok;
}

bool shaders(Builder& builder) {
    const std::filesystem::path folder = builder.binaries() / "Shaders";
    const std::string validator = findTool("spirv-val");
    const auto& formats = builder.shaderFormats();
    const auto has = [&](const char* format) { return std::find(formats.begin(), formats.end(), format) != formats.end(); };
    std::vector<std::string> problems;
    int checked = 0;
    for (const auto& entry : std::filesystem::directory_iterator(builder.root() / "Shaders")) {
        if (entry.path().extension() != ".hlsl") continue;
        const std::string name = entry.path().stem().string();
        for (const char* stage : {"vs", "ps"}) {
            const std::string base = name + "." + stage;
            const std::filesystem::path spv = folder / (base + ".spv");
            if (!std::filesystem::exists(spv)) {
                problems.push_back("missing " + base + ".spv");
            } else {
                ++checked;
                std::ifstream in(spv, std::ios::binary);
                unsigned char magic[4] = {};
                in.read(reinterpret_cast<char*>(magic), 4);
                if (!(magic[0] == 0x03 && magic[1] == 0x02 && magic[2] == 0x23 && magic[3] == 0x07)) {
                    problems.push_back(base + ".spv is not SPIR-V");
                }
                std::string output;
                if (!validator.empty() && !run({validator, spv.string()}, &output)) {
                    problems.push_back(base + ".spv failed spirv-val: " + output);
                }
            }
            if (has("dxil") && !std::filesystem::exists(folder / (base + ".dxil"))) problems.push_back("missing " + base + ".dxil");
            if (has("msl") && !std::filesystem::exists(folder / (base + ".metal"))) problems.push_back("missing " + base + ".metal");
        }
        if (has("msl") && std::filesystem::exists(folder / (name + ".vs.air")) && !std::filesystem::exists(folder / (name + ".metallib"))) {
            problems.push_back("missing " + name + ".metallib");
        }
    }
    for (const std::string& problem : problems) std::printf("%s\n", problem.c_str());
    std::printf("checked %d SPIR-V modules%s\n", checked, validator.empty() ? " (magic numbers only; no spirv-val)" : "");
    return problems.empty();
}

bool shipping(Builder& builder) {
    std::string symbols;
    if (!run({"nm", "-a", (builder.binaries() / "player").string()}, &symbols)) return false;
    std::transform(symbols.begin(), symbols.end(), symbols.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (symbols.find("cinder3dev") != std::string::npos) {
        std::printf("player contains cinder::dev symbols: the engine/dev split is broken\n");
        return false;
    }
    return true;
}

bool unit(Builder& builder) { return run({(builder.binaries() / "tests").string(), "--test-case-exclude=hot reload*"}); }

bool hotReload(Builder& builder) { return run({(builder.binaries() / "tests").string(), "--test-case=hot reload*"}); }

}

int runTests(Builder& builder, const std::vector<std::string>& names) {
    const std::vector<std::pair<std::string, std::function<bool(Builder&)>>> all{
            {"layers", layers},   {"layer_fixtures", layerFixtures}, {"shaders", shaders},
            {"shipping", shipping}, {"unit", unit},                  {"hot_reload", hotReload},
    };
    for (const std::string& name : names) {
        if (std::none_of(all.begin(), all.end(), [&](const auto& test) { return test.first == name; })) {
            std::fprintf(stderr, "cbt: unknown test %s\n", name.c_str());
            return 2;
        }
    }
    if (!builder.build({"editor", "tests"})) return 1;

    int failed = 0;
    int ran = 0;
    for (const auto& [name, test] : all) {
        if (!names.empty() && std::find(names.begin(), names.end(), name) == names.end()) continue;
        std::printf("== %s\n", name.c_str());
        std::fflush(stdout);
        const bool ok = test(builder);
        std::printf("== %s %s\n", name.c_str(), ok ? "passed" : "FAILED");
        ++ran;
        if (!ok) ++failed;
    }
    std::printf("cbt: %d of %d tests passed\n", ran - failed, ran);
    return failed == 0 ? 0 : 1;
}

}
