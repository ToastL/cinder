#include "components/Camera.hpp"
#include "components/MeshPart.hpp"
#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "dev/Console.hpp"
#include "dev/Dockspace.hpp"
#include "dev/Explorer.hpp"
#include "dev/History.hpp"
#include "dev/ImGuiLayer.hpp"
#include "dev/Manipulator.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Properties.hpp"
#include "dev/Selection.hpp"
#include "dev/Toolbar.hpp"
#include "dev/Viewport.hpp"
#include "physics/Body.hpp"
#include "physics/Collider.hpp"
#include "platform/Assets.hpp"
#include "platform/Files.hpp"
#include "platform/Glfw.hpp"
#include "serial/SceneCodec.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void exercise() {
    using namespace cinder::dev;
    using cinder::serial::SceneCodec;

    const auto output = std::filesystem::path(CINDER_BINARY_DIR) / "graphics-smoke";
    std::filesystem::create_directories(output);
    cinder::platform::setProjectRoot(std::filesystem::path(CINDER_SOURCE_DIR) / "samples/sandbox3d");
    cinder::core::ProjectConfig config("Cinder graphics smoke", 960, 640, "", 60);
    cinder::core::Engine engine(config, overlayFactory());
    auto& scene = engine.scene();
    auto* camera = scene.create<cinder::components::Camera>(nullptr);
    camera->transform()->setPosition(0.0f, 3.0f, 10.0f);
    auto* body = scene.create<cinder::physics::Body>(nullptr);
    body->setName("Ball");
    body->transform()->setPosition(0.0f, 3.0f, 0.0f);
    scene.create<cinder::physics::Collider>(body);
    scene.create<cinder::components::MeshPart>(body);
    const int id = body->id();

    PlaySession session(engine);
    Selection selection;
    History history(scene);
    Console console(engine.script(), history, selection);
    Toolbar toolbar(session, history, selection, output / "saved.scene");
    Viewport viewport(session, selection, history, engine);
    Explorer explorer(selection, history, scene);
    Properties properties(selection, history, scene);
    engine.renderer().setOverlayDraw([&] {
        toolbar.draw();
        drawDockspace();
        viewport.draw();
        explorer.draw();
        properties.draw();
        console.draw();
    });
    cinder::core::GameLoop loop(60);
    history.reset();
    selection.select(id);
    const std::string initial = SceneCodec::save(scene);
    for (int i = 0; i < 3; ++i) session.tick(loop);
    require(SceneCodec::save(scene) == initial, "Edit mode changed the scene");

    const GizmoView view = gizmoView(engine.renderer().camera(), engine.renderer().camera().size());
    const auto gizmo = gizmoFor(*body->transform(), Tool::Move, Space::World, view);
    require(gizmo.has_value(), "Selected body has no move gizmo");
    const auto point = toScreen(view, gizmo->pivot);
    require(point.has_value(), "Gizmo pivot is not visible");
    const auto geometry = shapes(*gizmo, view);
    require(hitHandle(geometry, view, *point) == Handle::View, "Gizmo hit testing disagrees with geometry");
    Manipulation manipulation;
    require(manipulation.begin(*body, *gizmo, Handle::View, view, *point), "Cannot begin gizmo drag");
    history.touch("Move", id);
    require(manipulation.drag(scene, view, *point + glm::vec2(30.0f, 0.0f), false), "Cannot drag gizmo");
    manipulation.end();
    history.settle(false);
    const std::string edited = SceneCodec::save(scene);
    require(edited != initial && history.dirty(), "Gizmo edit was not recorded");
    session.tick(loop);
    history.undo(selection);
    require(SceneCodec::save(scene) == initial, "Undo did not restore the scene");
    require(selection.resolve(scene) == scene.byId(id), "Undo lost selection");
    history.redo(selection);
    require(SceneCodec::save(scene) == edited, "Redo did not restore the edit");
    history.save(output / "saved.scene");
    require(!history.dirty(), "Save did not clear dirty state");
    require(cinder::platform::readTextFile(output / "saved.scene") == edited, "Saved scene text changed");

    for (int cycle = 0; cycle < 3; ++cycle) {
        session.togglePlay();
        session.tick(loop);
        require(session.state() == PlaySession::State::Playing, "Play did not start");
        session.togglePause();
        session.tick(loop);
        require(session.state() == PlaySession::State::Paused, "Pause did not stop simulation");
        const std::string paused = SceneCodec::save(scene);
        session.tick(loop);
        session.tick(loop);
        require(SceneCodec::save(scene) == paused, "Paused scene changed");
        const float before = scene.byId(id)->transform()->position().y;
        session.step();
        session.tick(loop);
        require(session.state() == PlaySession::State::Paused, "Step left paused mode");
        require(scene.byId(id)->transform()->position().y < before, "Step did not simulate gravity");
        session.togglePause();
        session.tick(loop);
        require(session.state() == PlaySession::State::Playing, "Resume did not start simulation");
        engine.script().eval("engine.quit()");
        require(engine.quitRequested(), "Rebooted Lua host lost its runtime context");
        session.tick(loop);
        require(session.state() == PlaySession::State::Edit, "Game quit did not stop Play");
        require(SceneCodec::save(scene) == edited, "Stop did not restore the edited scene");
    }

    for (int width : {800, 1100, 960}) {
        glfwSetWindowSize(engine.window().handle(), width, 640);
        for (int i = 0; i < 3; ++i) session.tick(loop);
    }
    engine.renderer().capture((output / "editor.png").string());
    require(std::filesystem::file_size(output / "editor.png") > 24, "Editor capture is empty");
    engine.renderer().setOverlayDraw(nullptr);
    for (const glm::ivec2 size : {glm::ivec2(480, 320), glm::ivec2(640, 400), glm::ivec2(0, 0)}) {
        engine.renderer().setViewportSize(size.x, size.y);
        loop.idle(engine);
        loop.idle(engine);
    }
    engine.renderer().capture((output / "composite.png").string());
    require(std::filesystem::file_size(output / "composite.png") > 24, "Composite capture is empty");
}

}

int main(int, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);
    bool acquired = false;
    try {
        cinder::platform::Glfw::acquire();
        acquired = true;
        exercise();
        cinder::platform::Glfw::release();
        acquired = false;
        std::puts("graphics smoke: ALL PASS");
        return 0;
    } catch (const std::exception& error) {
        if (acquired) cinder::platform::Glfw::release();
        std::fprintf(stderr, "graphics smoke: %s\n", error.what());
        return 1;
    }
}
