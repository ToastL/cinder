#pragma once

#include <string>

namespace cinder::scene {
class Actor;
class Scene;
}

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
    void inspect(cinder::scene::Actor& actor);
    void attributes(cinder::scene::Actor& actor);
    void addAttribute(cinder::scene::Actor& actor);

    Selection& selection_;
    cinder::scene::Scene& scene_;
    std::string newName_;
    int newKind_ = 0;
};

}
