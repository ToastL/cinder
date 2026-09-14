#pragma once

#include <string>

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::dev {

class Selection;

class Properties {
public:
    static constexpr const char* TITLE = "Properties";

    Properties(Selection& selection, cinder::scene::Scene& scene);

    Properties(const Properties&) = delete;
    Properties& operator=(const Properties&) = delete;

    void draw();

private:
    void inspect(cinder::scene::Node& node);
    void attributes(cinder::scene::Node& node);
    void addAttribute(cinder::scene::Node& node);

    Selection& selection_;
    cinder::scene::Scene& scene_;
    std::string newName_;
    int newKind_ = 0;
};

}
