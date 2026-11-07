#include "Marker.hpp"
#include "game/Game.hpp"

#include <cmath>
#include <optional>
#include <string>

using namespace cinder;

namespace {

const reflect::PropDef* propOf(scene::Node& node, std::string_view name) {
    for (const reflect::PropDef& def : node.propList()) {
        if (def.name() == name) return &def;
    }
    return nullptr;
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

}

class SelfTest final : public scene::Node {
public:
    CINDER_NODE(SelfTest, scene::Node) {}

    void onStart() override {
        scene::Scene& world = game::scene();
        check("parent", parent() != nullptr && parent()->name() == "SelfTest");

        auto* a = world.create<nodes::Group>(nullptr);
        check("create", a != nullptr && world.types().nameOf(*a) == "Group" && a->name() == "Group");
        a->setName("Renamed");
        check("find by name", world.find("Renamed") == a);

        a->transform()->setPosition(1.0f, 2.0f, 3.0f);
        a->transform()->translate(0.0f, 1.0f, 0.0f);
        check("position", a->transform()->position() == glm::vec3(1.0f, 3.0f, 3.0f));

        scene::Node* kid = world.create("Group", a);
        kid->setName("Kid");
        check("child by class name", kid->parent() == a && a->children().size() == 1);
        check("findFirstChild", a->findFirstChild("Kid") == kid && a->findFirstChild("Nope") == nullptr);
        kid->setParent(nullptr);
        check("reparent to root", kid->parent() == nullptr);

        auto* sprite = world.create<nodes::Sprite>(a);
        const reflect::PropDef* texture = propOf(*sprite, "texture");
        texture->writeText(sprite->propTarget(), "none.png");
        check("string prop through reflection", texture->readText(sprite->propTarget()) == "none.png");

        auto* camera = world.create<nodes::Camera>(a);
        const reflect::PropDef* fov = propOf(*camera, "fov");
        const float wide = 500.0f;
        fov->write(camera->propTarget(), &wide);
        float clamped = 0.0f;
        fov->read(camera->propTarget(), &clamped);
        check("reflection clamps", near(clamped, 179.0f));
        propOf(*camera, "projection")->writeText(camera->propTarget(), "orthographic");
        check("enum prop", propOf(*camera, "projection")->readText(camera->propTarget()) == "orthographic");

        auto* holder = world.create<nodes::Group>(nullptr);
        holder->transform()->setPosition(10.0f, 0.0f, 0.0f);
        auto* inner = world.create<nodes::Group>(world.create<nodes::Folder>(holder));
        inner->transform()->setPosition(0.0f, 1.0f, 0.0f);
        check("folder breaks the chain", inner->transform()->worldPosition() == glm::vec3(0.0f, 1.0f, 0.0f));

        scene::Node* copy = world.clone(*sprite, a);
        check("clone", copy != sprite && copy->parent() == a
                               && propOf(*copy, "texture")->readText(copy->propTarget()) == "none.png");
        texture->writeText(sprite->propTarget(), "");
        texture->writeText(copy->propTarget(), "");

        a->setAttribute("hp", scene::PropValue::integer(10));
        check("attribute", a->attribute("hp") != nullptr && a->attribute("hp")->as<std::int64_t>() == 10);
        world.connect(*a, scene::Signal::AttributeChanged, *this, [this](const scene::SignalArgs& args) {
            if (args.name == "hp") hpChanges_++;
        });
        a->setAttribute("hp", scene::PropValue::integer(5));
        a->setAttribute("hp", scene::PropValue::integer(5));
        check("attribute changed fires once per change", hpChanges_ == 1);

        world.connect(*a, scene::Signal::ChildAdded, *this,
                      [this](const scene::SignalArgs& args) { added_ = args.other->id(); });
        scene::Node* temp = world.create<nodes::Group>(a);
        check("child added", added_ == temp->id());
        world.connect(*temp, scene::Signal::Destroying, *this, [this](const scene::SignalArgs&) { destroyed_ = true; });
        world.destroy(temp);

        spinner_ = world.create<nodes::Group>(nullptr)->id();
        world.every(*this, 0.0, [this] { ticks_++; });
        world.after(*this, 0.1, [this] { waited_ = true; });

        auto* spawned = world.create<nodes::Group>(nullptr);
        spawned->setName("Spawned");
        spawned->setAttribute("mark", scene::PropValue::integer(7));
        check("a module class by name", world.create("Marker", spawned) != nullptr);

        scene::Node* ticker = world.create("Ticker", world.create<nodes::Group>(nullptr));
        const int tickerOwner = ticker->parent()->id();
        world.after(*this, 0.1, [this, &world, tickerOwner] {
            world.destroy(world.byId(tickerOwner));
            world.after(*this, 0.05, [this] { ticksAtDestroy_ = selftestTicks(); });
        });

        auto* ground = world.create<physics::Collider>(nullptr);
        ground->setName("Ground");
        ground->setSize(20.0f, 1.0f, 20.0f);
        ground->transform()->setPosition(0.0f, -0.5f, 0.0f);

        auto* ball = world.create<physics::Body>(nullptr);
        ball->setName("Ball");
        ball->transform()->setPosition(0.0f, 0.8f, 0.0f);
        world.create<physics::Collider>(ball)->setShape(physics::Shape::Sphere);
        ball->applyImpulse(glm::vec3(0.0f, 0.0f, 0.5f));
        ball_ = ball->id();
        ground_ = ground->id();
        world.connect(*ball, scene::Signal::Touched, *this, [this](const scene::SignalArgs& args) {
            if (touchedBy_ == 0) touchedBy_ = args.other->id();
        });

        world.after(*this, 0.4, [this] { finish(); });
    }

    void onUpdate(float dt) override {
        if (scene::Node* spinner = game::scene().byId(spinner_)) {
            const glm::vec3 rotation = spinner->transform()->rotation();
            spinner->transform()->setRotation(rotation.x, rotation.y + dt, rotation.z);
        }
    }

private:
    void check(const std::string& what, bool ok) {
        if (!ok) fails_++;
        platform::logInfo("[game] %s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    }

    void finish() {
        scene::Scene& world = game::scene();
        check("every-update timer", ticks_ > 5);
        check("after timer", waited_);
        check("destroying signal", destroyed_);
        check("an update rotates a node", world.byId(spinner_)->transform()->rotation().y > 0.1f);
        check("attribute from the scene", selftestMarks()["Marked"] == 42.0);
        check("attribute set before the node started", selftestMarks()["Spawned"] == 7.0);
        check("a destroyed subtree stops updating",
              ticksAtDestroy_ > 0 && selftestTicks() == ticksAtDestroy_);

        scene::Node* ball = world.byId(ball_);
        const std::optional<physics::RayHit> hit =
                game::raycast(ball->transform()->position() + glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 10.0f);
        check("gravity pulls down", game::world().gravity().y < 0.0f);
        check("body rests on a collider", std::fabs(ball->transform()->position().y - 0.5f) < 0.05f);
        check("impulse moved the body", ball->transform()->position().z > 0.05f);
        check("touched fires with the other node", touchedBy_ == ground_);
        check("raycast finds the body", hit.has_value() && hit->node == ball && hit->normal.y > 0.9f);

        platform::logInfo("[game] %s\n", fails_ == 0 ? "ALL PASS" : (std::to_string(fails_) + " FAILED").c_str());
        game::quit();
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
