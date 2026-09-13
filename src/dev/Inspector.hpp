#pragma once

namespace cinder::scene { class Scene; }

namespace cinder::dev {

class Selection;

class Inspector {
public:
    static constexpr const char* TITLE = "Inspector";

    Inspector(Selection& selection, cinder::scene::Scene& scene);

    Inspector(const Inspector&) = delete;
    Inspector& operator=(const Inspector&) = delete;

    void draw();

private:
    Selection& selection_;
    cinder::scene::Scene& scene_;
};

}
