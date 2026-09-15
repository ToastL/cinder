#pragma once

#include <filesystem>

namespace cinder::dev {

class History;
class PlaySession;
class Selection;

class Toolbar {
public:
    Toolbar(PlaySession& session, History& history, Selection& selection, std::filesystem::path scene);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    void draw();
    void requestClose();
    bool closeConfirmed() const { return closeConfirmed_; }

private:
    bool save();
    void closePrompt();

    PlaySession& session_;
    History& history_;
    Selection& selection_;
    std::filesystem::path scene_;
    bool closeRequested_ = false;
    bool closeConfirmed_ = false;
};

}
