#pragma once

#include <filesystem>

namespace cinder::core { class Engine; }

namespace cinder::dev {

class PlaySession;

class Toolbar {
public:
    Toolbar(PlaySession& session, cinder::core::Engine& engine, std::filesystem::path scene);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    void draw();

private:
    void save();

    PlaySession& session_;
    cinder::core::Engine& engine_;
    std::filesystem::path scene_;
};

}
