#pragma once

namespace cinder::scene {
class Actor;
class Scene;
}

namespace cinder::dev {

class Selection;

class Hierarchy {
public:
    static constexpr const char* TITLE = "Hierarchy";

    Hierarchy(Selection& selection, cinder::scene::Scene& scene);

    Hierarchy(const Hierarchy&) = delete;
    Hierarchy& operator=(const Hierarchy&) = delete;

    void draw();

private:
    void node(cinder::scene::Actor& actor);

    Selection& selection_;
    cinder::scene::Scene& scene_;
};

}
