#include "CinderBuildTool/Rules.hpp"


namespace {

std::string quoted(const std::filesystem::path& path) { return "\"" + path.string() + "\""; }

std::string library(const cbt::TargetInfo& t, const std::string& name) {
    return quoted(t.binaries / (name + (t.platform == "macOS" ? ".dylib" : ".so")));
}

}

CINDER_TARGET("tests", [](cbt::TargetRules& t, const cbt::TargetInfo& info) {
    t.modules({"tests"}).exportSymbols().prerequisites({"test_module", "test_module_stale"});
    t.defines({"CINDER_SOURCE_DIR=" + quoted(info.root), "CINDER_TEST_MODULE=" + library(info, "test_module"),
               "CINDER_STALE_MODULE=" + library(info, "test_module_stale"),
               "CINDER_CBT=" + quoted(info.binaries.parent_path() / "cbt"),
               "CINDER_OUTPUT_ROOT=" + quoted(info.binaries.parent_path().parent_path().parent_path()),
               "CINDER_CONFIG=\"" + info.config + "\""});
});

CINDER_TARGET("test_module", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.type(cbt::TargetType::GameModule).modules({"test_module"});
});

CINDER_TARGET("test_module_stale", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.type(cbt::TargetType::GameModule).modules({"test_module"}).defines({"CINDER_MODULE_ABI=\"0.0.0-stale\""});
});

CINDER_TARGET("ui_gallery", [](cbt::TargetRules& t, const cbt::TargetInfo&) {
    t.modules({"gallery"});
});

CINDER_TARGET("graphics_smoke", [](cbt::TargetRules& t, const cbt::TargetInfo& info) {
    t.modules({"graphics_smoke"});
    t.defines({"CINDER_SOURCE_DIR=" + quoted(info.root), "CINDER_BINARY_DIR=" + quoted(info.intermediate)});
});
