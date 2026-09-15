#include "dev/History.hpp"

#include "dev/Selection.hpp"
#include "serial/SceneCodec.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace cinder::dev {
namespace {

const std::string NO_LABEL;

}

History::History(cinder::scene::Scene& scene) : scene_(scene) {}

void History::reset() {
    current_ = cinder::serial::SceneCodec::save(scene_);
    saved_ = current_;
    undo_.clear();
    redo_.clear();
    pending_ = false;
    dirty_ = false;
}

void History::touch(std::string label, std::optional<int> selection) {
    if (!enabled_ || pending_) return;
    pending_ = true;
    label_ = std::move(label);
    selection_ = selection;
}

void History::settle(bool interacting) {
    if (enabled_ && pending_ && !interacting) commit();
}

const std::string& History::undoLabel() const { return undo_.empty() ? NO_LABEL : undo_.back().label; }

const std::string& History::redoLabel() const { return redo_.empty() ? NO_LABEL : redo_.back().label; }

void History::undo(Selection& selection) { restore(undo_, redo_, selection); }

void History::redo(Selection& selection) { restore(redo_, undo_, selection); }

void History::save(const std::filesystem::path& path) {
    if (enabled_ && pending_) commit();
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());

    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + path.string());
    out << current_;
    out.close();
    if (!out) throw std::runtime_error("Cannot write " + path.string());

    saved_ = current_;
    dirty_ = false;
}

void History::commit() {
    pending_ = false;
    std::string next = cinder::serial::SceneCodec::save(scene_);
    if (next == current_) return;

    undo_.push_back(Step{std::move(label_), std::move(current_), selection_});
    if (undo_.size() > MAX_STEPS) undo_.erase(undo_.begin());
    redo_.clear();

    current_ = std::move(next);
    dirty_ = current_ != saved_;
}

void History::restore(std::vector<Step>& from, std::vector<Step>& to, Selection& selection) {
    if (!enabled_) return;
    if (pending_) commit();
    if (from.empty()) return;

    Step step = std::move(from.back());
    from.pop_back();
    to.push_back(Step{step.label, std::move(current_), selection.id()});

    cinder::serial::SceneCodec::load(step.scene, scene_);
    current_ = std::move(step.scene);
    dirty_ = current_ != saved_;

    if (step.selection) selection.select(*step.selection);
    else selection.clear();
}

}
