#include "Marker.hpp"
#include "Game/Game.hpp"

#include <cmath>
#include <optional>
#include <string>

using namespace cinder;

namespace {

const Reflect::PropDef* propOf(Scene::Object& object, std::string_view name) {
    for (const Reflect::PropDef& def : object.propList()) {
        if (def.name() == name) return &def;
    }
    return nullptr;
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

Scene::Actor* group(Scene::Scene& world, Scene::Actor* parent) {
    Scene::Actor* actor = world.spawn<Scene::Actor>(parent);
    actor->addComponent<Scene::SceneComponent>();
    return actor;
}

}

class SelfTest final : public Scene::Component {
public:
    CINDER_COMPONENT(SelfTest, Scene::Component) {}

    void beginPlay() override {
        Scene::Scene& world = Game::scene();
        check("owner", owner() != nullptr && owner()->name() == "SelfTest");

        Scene::Actor* a = group(world, nullptr);
        check("spawn", world.types().nameOf(*a) == "Actor" && a->name() == "Actor" && a->rootComponent() != nullptr);
        a->setName("Renamed");
        check("find by name", world.find("Renamed") == a);

        a->transform()->setPosition(1.0f, 2.0f, 3.0f);
        a->transform()->translate(0.0f, 1.0f, 0.0f);
        check("position", a->transform()->position() == glm::vec3(1.0f, 3.0f, 3.0f));

        Scene::Actor* kid = world.spawn("Actor", a);
        kid->setName("Kid");
        check("attached by class name", kid->attachParent() == a && a->attachedActors().size() == 1);
        check("findAttached", a->findAttached("Kid") == kid && a->findAttached("Nope") == nullptr);
        kid->attachTo(nullptr);
        check("detach to the root", kid->attachParent() == nullptr);

        auto* spriteActor = world.spawn<Components::SpriteActor>(a);
        Components::SpriteComponent& sprite = spriteActor->sprite();
        const Reflect::PropDef* texture = propOf(sprite, "texture");
        texture->writeText(sprite.propTarget(), "none.png");
        check("string prop through reflection", texture->readText(sprite.propTarget()) == "none.png");
        check("default component", sprite.isDefault() && sprite.owner() == spriteActor
                                           && spriteActor->rootComponent() == &sprite);

        Scene::Component* added = world.addComponent(*a, "CameraComponent");
        auto* camera = dynamic_cast<Components::CameraComponent*>(added);
        check("component by class name", camera != nullptr && !camera->isDefault() && camera->owner() == a);
        const Reflect::PropDef* fov = propOf(*camera, "fov");
        const float wide = 500.0f;
        fov->write(camera->propTarget(), &wide);
        float clamped = 0.0f;
        fov->read(camera->propTarget(), &clamped);
        check("reflection clamps", near(clamped, 179.0f));
        propOf(*camera, "projection")->writeText(camera->propTarget(), "orthographic");
        check("enum prop", propOf(*camera, "projection")->readText(camera->propTarget()) == "orthographic");
        camera->transform().setPosition(0.0f, 0.0f, 2.0f);
        check("a component follows the root", camera->transform().worldPosition() == glm::vec3(1.0f, 3.0f, 5.0f));

        Scene::Actor* holder = group(world, nullptr);
        holder->transform()->setPosition(10.0f, 0.0f, 0.0f);
        Scene::Actor* inner = group(world, holder);
        inner->transform()->setPosition(0.0f, 1.0f, 0.0f);
        check("an attached actor follows its parent", inner->transform()->worldPosition() == glm::vec3(10.0f, 1.0f, 0.0f));

        Scene::Actor* copy = world.clone(*spriteActor, a);
        auto* copied = copy != nullptr ? copy->findComponent<Components::SpriteComponent>() : nullptr;
        check("clone", copy != spriteActor && copy->attachParent() == a && copied != nullptr && copied != &sprite
                               && propOf(*copied, "texture")->readText(copied->propTarget()) == "none.png");
        texture->writeText(sprite.propTarget(), "");
        texture->writeText(copied->propTarget(), "");

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
        Scene::Actor* temp = group(world, a);
        check("child added", added_ == temp->id());
        world.connect(*temp, Scene::Signal::Destroying, *this, [this](const Scene::SignalArgs&) { destroyed_ = true; });
        world.destroy(temp);

        spinner_ = group(world, nullptr)->id();
        world.every(*this, 0.0, [this] { ticks_++; });
        world.after(*this, 0.1, [this] { waited_ = true; });

        Scene::Actor* spawned = group(world, nullptr);
        spawned->setName("Spawned");
        spawned->setAttribute("mark", Scene::PropValue::integer(7));
        check("a module class by name", world.addComponent(*spawned, "Marker") != nullptr);

        Scene::Actor* tickerOwner = group(world, nullptr);
        world.addComponent(*tickerOwner, "Ticker");
        const int tickerId = tickerOwner->id();
        world.after(*this, 0.1, [this, &world, tickerId] {
            world.destroy(world.actor(tickerId));
            world.after(*this, 0.05, [this] { ticksAtDestroy_ = selftestTicks(); });
        });

        Scene::Actor* ground = world.spawn<Scene::Actor>();
        ground->setName("Ground");
        auto* floor = ground->addComponent<Physics::ColliderComponent>();
        floor->setSize(20.0f, 1.0f, 20.0f);
        floor->transform().setPosition(0.0f, -0.5f, 0.0f);

        Scene::Actor* ball = group(world, nullptr);
        ball->setName("Ball");
        ball->transform()->setPosition(0.0f, 0.8f, 0.0f);
        auto* body = ball->addComponent<Physics::RigidBodyComponent>();
        auto* shape = ball->addComponent<Physics::ColliderComponent>();
        shape->setShape(Physics::Shape::Sphere);
        body->applyImpulse(glm::vec3(0.0f, 0.0f, 0.5f));
        ball_ = ball->id();
        ground_ = ground->id();
        world.connect(*ball, Scene::Signal::Touched, *this, [this](const Scene::SignalArgs& args) {
            if (touchedBy_ == 0) touchedBy_ = args.other->id();
        });
        world.connect(*shape, Scene::Signal::Touched, *this, [this](const Scene::SignalArgs& args) {
            if (shapeTouchedBy_ == 0) shapeTouchedBy_ = args.other->id();
        });

        world.after(*this, 0.4, [this] { finish(); });
    }

    void tick(float dt) override {
        if (Scene::Actor* spinner = Game::scene().actor(spinner_)) {
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
        check("a tick rotates an actor", world.actor(spinner_)->transform()->rotation().y > 0.1f);
        check("attribute from the scene", selftestMarks()["Marked"] == 42.0);
        check("attribute set before the actor started", selftestMarks()["Spawned"] == 7.0);
        check("a destroyed actor stops ticking",
              ticksAtDestroy_ > 0 && selftestTicks() == ticksAtDestroy_);

        Scene::Actor* ball = world.actor(ball_);
        const std::optional<Physics::RayHit> hit =
                Game::raycast(ball->transform()->position() + glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 10.0f);
        check("gravity pulls down", Game::world().gravity().y < 0.0f);
        check("body rests on a collider", std::fabs(ball->transform()->position().y - 0.5f) < 0.05f);
        check("impulse moved the body", ball->transform()->position().z > 0.05f);
        check("touched fires on the actor with the other actor", touchedBy_ == ground_);
        check("touched fires on the collider", shapeTouchedBy_ == ground_);
        check("raycast finds the body", hit.has_value() && hit->actor == ball && hit->normal.y > 0.9f);

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
    int shapeTouchedBy_ = 0;
};

CINDER_REGISTER(SelfTest);
