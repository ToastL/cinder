#include "dev/Toolbar.hpp"

#include "dev/History.hpp"
#include "dev/PlaySession.hpp"
#include "platform/Log.hpp"

#include <imgui.h>

#include <exception>
#include <utility>

namespace cinder::dev {
namespace {

constexpr const char* CLOSE_POPUP = "Unsaved Changes";

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

Toolbar::Toolbar(PlaySession& session, History& history, Selection& selection, std::filesystem::path scene)
    : session_(session), history_(history), selection_(selection), scene_(std::move(scene)) {}

void Toolbar::draw() {
    const PlaySession::State state = session_.state();
    const bool editing = state == PlaySession::State::Edit;
    const bool interacting = ImGui::IsAnyItemActive();

    history_.setEnabled(editing);
    history_.settle(interacting);

    if (pressed(ImGuiMod_Ctrl | ImGuiKey_P)) session_.togglePlay();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P)) session_.togglePause();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_P)) session_.step();
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_S) && editing) save();
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Z) && editing && !interacting) history_.undo(selection_);
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z) && editing && !interacting) {
        history_.redo(selection_);
    }

    closePrompt();

    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::Button(editing ? "Play" : "Stop")) session_.togglePlay();

    ImGui::BeginDisabled(editing);
    if (ImGui::Button(state == PlaySession::State::Paused ? "Resume" : "Pause")) {
        session_.togglePause();
    }
    if (ImGui::Button("Step")) session_.step();
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!editing);
    if (ImGui::Button("Save")) save();
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!editing || !history_.canUndo());
    if (ImGui::Button("Undo")) history_.undo(selection_);
    ImGui::EndDisabled();
    if (history_.canUndo()) ImGui::SetItemTooltip("Undo %s", history_.undoLabel().c_str());

    ImGui::BeginDisabled(!editing || !history_.canRedo());
    if (ImGui::Button("Redo")) history_.redo(selection_);
    ImGui::EndDisabled();
    if (history_.canRedo()) ImGui::SetItemTooltip("Redo %s", history_.redoLabel().c_str());

    ImGui::TextDisabled("%s  %s%s", label(state), scene_.filename().string().c_str(),
                        history_.dirty() ? "*" : "");
    ImGui::EndMainMenuBar();
}

void Toolbar::requestClose() {
    if (history_.dirty()) closeRequested_ = true;
    else closeConfirmed_ = true;
}

void Toolbar::closePrompt() {
    if (std::exchange(closeRequested_, false)) ImGui::OpenPopup(CLOSE_POPUP);
    if (!ImGui::BeginPopupModal(CLOSE_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    ImGui::Text("Save changes to %s before closing?", scene_.filename().string().c_str());
    if (ImGui::Button("Save")) {
        closeConfirmed_ = save();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save")) {
        closeConfirmed_ = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

bool Toolbar::save() {
    try {
        history_.save(scene_);
        cinder::platform::logInfo("[editor] saved %s\n", scene_.string().c_str());
        return true;
    } catch (const std::exception& e) {
        cinder::platform::logError("[editor] save failed: %s\n", e.what());
        return false;
    }
}

}
