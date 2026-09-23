#include "components/Camera.hpp"
#include "components/MeshPart.hpp"
#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "dev/History.hpp"
#include "dev/Manipulator.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "dev/panels/Console.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "dev/panels/Explorer.hpp"
#include "dev/panels/Layout.hpp"
#include "dev/panels/Properties.hpp"
#include "dev/panels/SceneView.hpp"
#include "dev/panels/Toolbar.hpp"
#include "physics/Body.hpp"
#include "physics/Collider.hpp"
#include "platform/Assets.hpp"
#include "platform/Files.hpp"
#include "platform/Glfw.hpp"
#include "platform/InputEvent.hpp"
#include "serial/SceneCodec.hpp"
#include "text/FontSet.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/docking/DockTab.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/framework/Application.hpp"
#include "ui/framework/WindowPlatform.hpp"
#include "ui/widgets/TreeView.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using cinder::platform::InputEvent;
using cinder::platform::InputEventType;
namespace buttons = cinder::platform::buttons;
namespace keys = cinder::platform::keys;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Driver {
public:
    explicit Driver(cinder::platform::Input& input) : input_(&input) {
        input_->setRecording(true);
        input_->setIgnoreSystem(true);
    }

    void move(glm::vec2 to) {
        cursor_ = to;
        InputEvent event;
        event.type = InputEventType::MouseMove;
        event.position = cursor_;
        input_->inject(event);
    }

    void button(InputEventType type, int code) {
        InputEvent event;
        event.type = type;
        event.code = code;
        event.position = cursor_;
        input_->inject(event);
    }

    void press(int code = buttons::LEFT) { button(InputEventType::MouseDown, code); }
    void release(int code = buttons::LEFT) { button(InputEventType::MouseUp, code); }

    void key(InputEventType type, int code) {
        InputEvent event;
        event.type = type;
        event.code = code;
        input_->inject(event);
    }

    void chord(int code, bool shift = false) {
#if defined(__APPLE__)
        const int primary = keys::LEFT_SUPER;
#else
        const int primary = keys::LEFT_CONTROL;
#endif
        key(InputEventType::KeyDown, primary);
        if (shift) key(InputEventType::KeyDown, keys::LEFT_SHIFT);
        key(InputEventType::KeyDown, code);
        key(InputEventType::KeyUp, code);
        if (shift) key(InputEventType::KeyUp, keys::LEFT_SHIFT);
        key(InputEventType::KeyUp, primary);
    }

private:
    cinder::platform::Input* input_;
    glm::vec2 cursor_{0.0f};
};

glm::vec2 centreOf(const cinder::ui::Application& app, const std::shared_ptr<cinder::ui::Widget>& widget) {
    const cinder::ui::Geometry geometry = app.grid().geometryOf(widget.get()).value_or(cinder::ui::Geometry{});
    require(geometry.size.x > 0.0f && geometry.size.y > 0.0f, "Widget was not painted");
    return geometry.rect().center();
}

void exercise() {
    using namespace cinder::dev;
    using cinder::serial::SceneCodec;

    const auto output = std::filesystem::path(CINDER_BINARY_DIR) / "graphics-smoke";
    std::filesystem::create_directories(output);
    cinder::platform::setProjectRoot(std::filesystem::path(CINDER_SOURCE_DIR) / "samples/sandbox3d");
    cinder::core::ProjectConfig config("Cinder graphics smoke", 1100, 700, "", 60);
    cinder::core::Engine engine(config);
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
    const cinder::text::FontSet fonts = cinder::text::FontSet::engineDefault();
    cinder::ui::WindowPlatform platform(engine.window());
    cinder::ui::Application app(platform, fonts, panels::editorTheme());
    panels::Toolbar toolbar(session, history, selection, output / "saved.scene", app);
    panels::SceneView view(session, selection, history, engine, app);
    panels::Explorer explorer(selection, history, scene, app);
    panels::Properties properties(selection, history, scene, app);
    panels::Console console(engine.script(), history, selection, app);
    cinder::ui::TabManager tabs;
    app.setRoot(panels::editorLayout(toolbar, view, explorer, properties, console, tabs));

    cinder::platform::Input& input = engine.input();
    Driver driver(input);
    engine.renderer().setUiPaint([&](cinder::ui::ElementList& list) {
        const std::vector<InputEvent> events = input.takeEvents();
        app.processEvents(events);
        toolbar.update();
        view.update();
        explorer.update();
        properties.update();
        console.update();
        app.paint(list);
        view.afterPaint();
    });

    cinder::core::GameLoop loop(60);
    const auto frames = [&](int count) {
        for (int i = 0; i < count; ++i) session.tick(loop);
    };

    history.reset();
    const std::string initial = SceneCodec::save(scene);
    frames(3);
    require(SceneCodec::save(scene) == initial, "Edit mode changed the scene");
    require(tabs.stackCount() == 4, "The editor did not dock its four panels");

    const std::shared_ptr<cinder::ui::TableRow> row = explorer.tree()->rowFor(static_cast<cinder::ui::ItemId>(id));
    require(row != nullptr, "The Explorer has no row for the body");
    driver.move(centreOf(app, row));
    driver.press();
    driver.release();
    frames(2);
    require(selection.id() == id, "Clicking the Explorer row did not select the node");

    const GizmoView gizmo = gizmoView(engine.renderer().camera(), engine.renderer().camera().size());
    const auto placed = gizmoFor(*body->transform(), Tool::Move, Space::World, gizmo);
    require(placed.has_value(), "Selected body has no move gizmo");
    const auto pivot = toScreen(gizmo, placed->pivot);
    require(pivot.has_value(), "Gizmo pivot is not visible");
    const cinder::ui::Geometry panel =
            app.grid().geometryOf(view.viewport().get()).value_or(cinder::ui::Geometry{});
    const glm::vec2 handle = panel.position + *pivot;

    driver.move(handle);
    driver.press();
    driver.move(handle + glm::vec2(30.0f, 0.0f));
    driver.release();
    frames(3);
    const std::string edited = SceneCodec::save(scene);
    require(edited != initial, "Dragging the gizmo did not move the node");
    require(history.dirty(), "The gizmo drag was not recorded");
    require(history.canUndo(), "The gizmo drag left nothing to undo");

    driver.chord(keys::letter('z'));
    frames(2);
    require(SceneCodec::save(scene) == initial, "Undo did not restore the scene");
    require(selection.resolve(scene) == scene.byId(id), "Undo lost the selection");
    driver.chord(keys::letter('z'), true);
    frames(2);
    require(SceneCodec::save(scene) == edited, "Redo did not restore the edit");

    history.save(output / "saved.scene");
    require(!history.dirty(), "Save did not clear the dirty state");
    require(cinder::platform::readTextFile(output / "saved.scene") == edited, "Saved scene text changed");

    driver.chord(keys::letter('p'));
    frames(2);
    require(session.state() == PlaySession::State::Playing, "The Play shortcut did not start the game");
    driver.chord(keys::letter('p'));
    frames(2);
    require(session.state() == PlaySession::State::Edit, "The Play shortcut did not stop the game");
    require(SceneCodec::save(scene) == edited, "Stop did not restore the edited scene");

    for (int cycle = 0; cycle < 3; ++cycle) {
        session.togglePlay();
        frames(1);
        require(session.state() == PlaySession::State::Playing, "Play did not start");
        session.togglePause();
        frames(1);
        require(session.state() == PlaySession::State::Paused, "Pause did not stop simulation");
        const std::string paused = SceneCodec::save(scene);
        frames(2);
        require(SceneCodec::save(scene) == paused, "Paused scene changed");
        const float before = scene.byId(id)->transform()->position().y;
        session.step();
        frames(1);
        require(session.state() == PlaySession::State::Paused, "Step left paused mode");
        require(scene.byId(id)->transform()->position().y < before, "Step did not simulate gravity");
        session.togglePause();
        frames(1);
        require(session.state() == PlaySession::State::Playing, "Resume did not start simulation");
        engine.script().eval("engine.quit()");
        require(engine.quitRequested(), "Rebooted Lua host lost its runtime context");
        frames(1);
        require(session.state() == PlaySession::State::Edit, "Game quit did not stop Play");
        require(SceneCodec::save(scene) == edited, "Stop did not restore the edited scene");
    }

    const std::shared_ptr<cinder::ui::TabStack> stack = tabs.stackWidget(tabs.stackOf("Console"));
    require(stack != nullptr, "The docked layout lost the console's stack");
    const glm::vec2 consoleTab = centreOf(app, stack->tabWidget("Console"));
    driver.move(consoleTab);
    driver.press();
    driver.move(consoleTab + glm::vec2(14.0f, 6.0f));
    frames(1);
    driver.move(centreOf(app, view.viewport()));
    frames(1);
    driver.release();
    frames(2);
    require(tabs.isOpen("Console"), "Re-docking the console closed it");
    require(tabs.stackCount() == 3, "Re-docking the console did not join the Scene stack");

    for (int width : {900, 1200, 1100}) {
        glfwSetWindowSize(engine.window().handle(), width, 700);
        frames(3);
    }
    engine.renderer().requestWindowCapture((output / "editor.png").string());
    frames(2);
    require(std::filesystem::file_size(output / "editor.png") > 24, "Editor capture is empty");

    engine.renderer().setUiPaint(nullptr);
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
