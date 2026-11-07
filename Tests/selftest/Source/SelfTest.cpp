#include "Marker.hpp"
#include "Game/Game.hpp"

#include <cmath>
#include <optional>
#include <string>

using namespace cinder;

namespace {

const Reflect::PropDef* propOf(Scene::Node& node, std::string_view name) {
    for (const Reflect::PropDef& def : node.propList()) {
        if (def.name() == name) return &def;
    }
    return nullptr;
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

}

class SelfTest final : public Scene::Node {
public:
    CINDER_NODE(SelfTest, Scene::Node) {}

    void onStart() override {
        Scene::Scene& world = Game::scene();
        check("parent", parent() != nullptr && parent()->name() == "SelfTest");

        auto* a = world.create<Nodes::Group>(nullptr);
        check("create", a != nullptr && world.types().nameOf(*a) == "Group" && a->name() == "Group");
        a->setName("Renamed");
        check("find by name", world.find("Renamed") == a);

        a->transform()->setPosition(1.0f, 2.0f, 3.0f);
        a->transform()->translate(0.0f, 1.0f, 0.0f);
        check("position", a->transform()->position() == glm::vec3(1.0f, 3.0f, 3.0f));

        Scene::Node* kid = world.create("Group", a);
        kid->setName("Kid");
        check("child by class name", kid->parent() == a && a->children().size() == 1);
        check("findFirstChild", a->findFirstChild("Kid") == kid && a->findFirstChild("Nope") == nullptr);
        kid->setParent(nullptr);
        check("reparent to root", kid->parent() == nullptr);

        auto* sprite = world.create<Nodes::Sprite>(a);
        const Reflect::PropDef* texture = propOf(*sprite, "texture");
        texture->writeText(sprite->propTarget(), "none.png");
        check("string prop through reflection", texture->readText(sprite->propTarget()) == "none.png");

        auto* camera = world.create<Nodes::Camera>(a);
        const Reflect::PropDef* fov = propOf(*camera, "fov");
        const float wide = 500.0f;
        fov->write(camera->propTarget(), &wide);
        float clamped = 0.0f;
        fov->read(camera->propTarget(), &clamped);
        check("reflection clamps", near(clamped, 179.0f));
        propOf(*camera, "projection")->writeText(camera->propTarget(), "orthographic");
        check("enum prop", propOf(*camera, "projection")->readText(camera->propTarget()) == "orthographic");

        auto* holder = world.create<Nodes::Group>(nullptr);
        holder->transform()->setPosition(10.0f, 0.0f, 0.0f);
        auto* inner = world.create<Nodes::Group>(world.create<Nodes::Folder>(holder));
        inner->transform()->setPosition(0.0f, 1.0f, 0.0f);
        check("folder breaks the chain", inner->transform()->worldPosition() == glm::vec3(0.0f, 1.0f, 0.0f));

        Scene::Node* copy = world.clone(*sprite, a);
        check("clone", copy != sprite && copy->parent() == a
                               && propOf(*copy, "texture")->readText(copy->propTarget()) == "none.png");
        texture->writeText(sprite->propTarget(), "");
        texture->writeText(copy->propTarget(), "");

        a->setAttribute("hp", Scene::PropValue::integer(10));
        check("attribute", a->attribute("hp") != nullptr && a->attribute("hp")->as<std::int64_t>() == 10);
        world.connect(*a, Scene::Signal::AttributeChanged, *this, [this](const Scene::SignalArgs& args) {
            if (args.name == "hp") hpChanges_++;
        });
        a->setAttribute("hp", Scene::PropValue::integer(5));
        a->setAttribute("hp", Scene::PropValue::integer(5));
        check("attribute changed fires once per change", hpChanges_ == 1);

        world.connect(*a, Scene::Signal::ChildAdded, *this,
                      [this](const Scene::SignalArgs& args) { added_ = args.other->id(); });
        Scene::Node* temp = world.create<Nodes::Group>(a);
        check("child added", added_ == temp->id());
        world.connect(*temp, Scene::Signal::Destroying, *this, [this](const Scene::SignalArgs&) { destroyed_ = true; });
        world.destroy(temp);

        spinner_ = world.create<Nodes::Group>(nullptr)->id();
        world.every(*this, 0.0, [this] { ticks_++; });
        world.after(*this, 0.1, [this] { waited_ = true; });

        auto* spawned = world.create<Nodes::Group>(nullptr);
        spawned->setName("Spawned");
        spawned->setAttribute("mark", Scene::PropValue::integer(7));
        check("a module class by name", world.create("Marker", spawned) != nullptr);

        Scene::Node* ticker = world.create("Ticker", world.create<Nodes::Group>(nullptr));
        const int tickerOwner = ticker->parent()->id();
        world.after(*this, 0.1, [this, &world, tickerOwner] {
            world.destroy(world.byId(tickerOwner));
            world.after(*this, 0.05, [this] { ticksAtDestroy_ = selftestTicks(); });
        });

        auto* ground = world.create<Physics::Collider>(nullptr);
        ground->setName("Ground");
        ground->setSize(20.0f, 1.0f, 20.0f);
        ground->transform()->setPosition(0.0f, -0.5f, 0.0f);

        auto* ball = world.create<Physics::Body>(nullptr);
        ball->setName("Ball");
        ball->transform()->setPosition(0.0f, 0.8f, 0.0f);
        world.create<Physics::Collider>(ball)->setShape(Physics::Shape::Sphere);
        ball->applyImpulse(glm::vec3(0.0f, 0.0f, 0.5f));
        ball_ = ball->id();
        ground_ = ground->id();
        world.connect(*ball, Scene::Signal::Touched, *this, [this](const Scene::SignalArgs& args) {
            if (touchedBy_ == 0) touchedBy_ = args.other->id();
        });

        world.after(*this, 0.4, [this] { finish(); });
    }

    void onUpdate(float dt) override {
        if (Scene::Node* spinner = Game::scene().byId(spinner_)) {
            const glm::vec3 rotation = spinner->transform()->rotation();
            spinner->transform()->setRotation(rotation.x, rotation.y + dt, rotation.z);
        }
    }

private:
    void check(const std::string& what, bool ok) {
        if (!ok) fails_++;
        Platform::logInfo("[game] %s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    }

    void finish() {
        Scene::Scene& world = Game::scene();
        check("every-update timer", ticks_ > 5);
        check("after timer", waited_);
        check("destroying signal", destroyed_);
        check("an update rotates a node", world.byId(spinner_)->transform()->rotation().y > 0.1f);
        check("attribute from the scene", selftestMarks()["Marked"] == 42.0);
        check("attribute set before the node started", selftestMarks()["Spawned"] == 7.0);
        check("a destroyed subtree stops updating",
              ticksAtDestroy_ > 0 && selftestTicks() == ticksAtDestroy_);

        Scene::Node* ball = world.byId(ball_);
        const std::optional<Physics::RayHit> hit =
                Game::raycast(ball->transform()->position() + glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 10.0f);
        check("gravity pulls down", Game::world().gravity().y < 0.0f);
        check("body rests on a collider", std::fabs(ball->transform()->position().y - 0.5f) < 0.05f);
        check("impulse moved the body", ball->transform()->position().z > 0.05f);
        check("touched fires with the other node", touchedBy_ == ground_);
        check("raycast finds the body", hit.has_value() && hit->node == ball && hit->normal.y > 0.9f);

        Platform::logInfo("[game] %s\n", fails_ == 0 ? "ALL PASS" : (std::to_string(fails_) + " FAILED").c_str());
        Game::quit();
    }

    int fails_ = 0;
    int hpChanges_ = 0;
    int added_ = 0;
    bool destroyed_ = false;
    int spinner_ = 0;
    int ticks_ = 0;
    bool waited_ = false;
    int ticksAtDestroy_ = 0;
    int ball_ = 0;
    int ground_ = 0;
    int touchedBy_ = 0;
};

CINDER_REGISTER(SelfTest);
