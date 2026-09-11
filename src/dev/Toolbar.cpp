#include "dev/Toolbar.hpp"

#include "core/Engine.hpp"
#include "dev/PlaySession.hpp"
#include "platform/Log.hpp"

#include <imgui.h>

#include <exception>
#include <utility>

namespace cinder::dev {
namespace {

const char* label(PlaySession::State state) {
    switch (state) {
        case PlaySession::State::Edit: return "Edit";
        case PlaySession::State::Playing: return "Playing";
        case PlaySession::State::Paused: return "Paused";
    }
    return "";
}

bool pressed(ImGuiKeyChord chord) { return ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal); }

}

Toolbar::Toolbar(PlaySession& session, cinder::core::Engine& engine, std::filesystem::path scene)
    : session_(session), engine_(engine), scene_(std::move(scene)) {}

void Toolbar::draw() {
    const PlaySession::State state = session_.state();
    const bool editing = state == PlaySession::State::Edit;

    if (pressed(ImGuiMod_Ctrl | ImGuiKey_P)) session_.togglePlay();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P)) session_.togglePause();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_P)) session_.step();
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_S) && editing) save();

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Toolbar", nullptr,
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button(editing ? "Play" : "Stop")) session_.togglePlay();
    ImGui::SameLine();

    ImGui::BeginDisabled(editing);
    if (ImGui::Button(state == PlaySession::State::Paused ? "Resume" : "Pause")) {
        session_.togglePause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) session_.step();
    ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::BeginDisabled(!editing);
    if (ImGui::Button("Save")) save();
    ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::TextDisabled("%s  %s", label(state), scene_.filename().string().c_str());
    ImGui::End();
}

void Toolbar::save() {
    try {
        engine_.saveScene(scene_);
        cinder::platform::logInfo("[editor] saved %s\n", scene_.string().c_str());
    } catch (const std::exception& e) {
        cinder::platform::logError("[editor] save failed: %s\n", e.what());
    }
}

}
