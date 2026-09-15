#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cinder::scene { class Scene; }

namespace cinder::dev {

class Selection;

class History {
public:
    static constexpr std::size_t MAX_STEPS = 100;

    explicit History(cinder::scene::Scene& scene);

    History(const History&) = delete;
    History& operator=(const History&) = delete;

    void reset();
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void touch(std::string label, std::optional<int> selection);
    void settle(bool interacting);

    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    const std::string& undoLabel() const;
    const std::string& redoLabel() const;
    std::size_t steps() const { return undo_.size(); }

    void undo(Selection& selection);
    void redo(Selection& selection);

    bool dirty() const { return dirty_; }
    const std::string& document() const { return current_; }
    void save(const std::filesystem::path& path);

private:
    struct Step {
        std::string label;
        std::string scene;
        std::optional<int> selection;
    };

    void commit();
    void restore(std::vector<Step>& from, std::vector<Step>& to, Selection& selection);

    cinder::scene::Scene& scene_;
    std::vector<Step> undo_;
    std::vector<Step> redo_;
    std::string current_;
    std::string saved_;
    std::string label_;
    std::optional<int> selection_;
    bool pending_ = false;
    bool enabled_ = true;
    bool dirty_ = false;
};

}
