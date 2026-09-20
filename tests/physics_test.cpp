#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Group.hpp"
#include "physics/Body.hpp"
#include "physics/Broadphase.hpp"
#include "physics/Collide.hpp"
#include "physics/Collider.hpp"
#include "physics/Geometry.hpp"
#include "physics/Nodes.hpp"
#include "physics/Pose.hpp"
#include "physics/World.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "serial/SceneCodec.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

using cinder::physics::Body;
using cinder::physics::Collider;
using cinder::physics::Geometry;
using cinder::physics::Manifold;
using cinder::physics::Shape;
using cinder::physics::World;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;

namespace {

constexpr float DT = 1.0f / 60.0f;

struct Fixture {
    NodeTypes types;
    Scene scene{types};
    World world{scene};

    Fixture() {
        cinder::components::registerBuiltins(types);
        cinder::physics::registerNodes(types);
    }

    void run(int steps) {
        for (int i = 0; i < steps; ++i) {
            scene.update(DT);
            world.step(DT);
        }
    }

    Collider* floor(Node* parent = nullptr) {
        Collider* collider = scene.create<Collider>(parent);
        collider->setSize(20.0f, 1.0f, 20.0f);
        collider->transform()->setPosition(0.0f, -0.5f, 0.0f);
        return collider;
    }

    Body* ball(float x, float y, float z, Node* parent = nullptr) {
        Body* body = scene.create<Body>(parent);
        body->transform()->setPosition(x, y, z);
        scene.create<Collider>(body)->setShape(Shape::Sphere);
        return body;
    }

    Body* crate(float x, float y, float z) {
        Body* body = scene.create<Body>(nullptr);
        body->transform()->setPosition(x, y, z);
        scene.create<Collider>(body);
        return body;
    }
};

Geometry sphereAt(const glm::vec3& center, float radius) {
    Geometry geometry;
    geometry.shape = Shape::Sphere;
    geometry.center = center;
    geometry.radius = radius;
    geometry.halfExtents = glm::vec3(radius);
    return geometry;
}

Geometry boxAt(const glm::vec3& center, const glm::vec3& halfExtents) {
    Geometry geometry;
    geometry.center = center;
    geometry.halfExtents = halfExtents;
    return geometry;
}

void checkVec(const glm::vec3& actual, const glm::vec3& expected, double epsilon = 1e-4) {
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(epsilon));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(epsilon));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(epsilon));
}

glm::vec3 worldOf(Node* node) { return node->transform()->worldPosition(); }

}

TEST_CASE("a box collider's geometry follows its world transform") {
    const glm::mat4 world = glm::scale(
        glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)), glm::half_pi<float>(),
                    glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::vec3(2.0f, 3.0f, 4.0f));

    const Geometry box = cinder::physics::geometryOf(Shape::Box, glm::vec3(1.0f, 2.0f, 1.0f), world);
    checkVec(box.center, glm::vec3(1.0f, 2.0f, 3.0f));
    checkVec(box.halfExtents, glm::vec3(1.0f, 3.0f, 2.0f));
    checkVec(box.axes[0], glm::vec3(0.0f, 1.0f, 0.0f));

    const cinder::physics::Bounds bounds = cinder::physics::boundsOf(box);
    checkVec(bounds.min, glm::vec3(-2.0f, 1.0f, 1.0f));
    checkVec(bounds.max, glm::vec3(4.0f, 3.0f, 5.0f));
}

TEST_CASE("a sphere collider fits the largest side of its scaled size") {
    const glm::mat4 world = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 3.0f, 2.0f));
    const Geometry sphere = cinder::physics::geometryOf(Shape::Sphere, glm::vec3(1.0f), world);
    CHECK(sphere.radius == doctest::Approx(1.5f));
}

TEST_CASE("overlapping spheres touch along the line between their centres") {
    Manifold manifold;
    REQUIRE(cinder::physics::collide(sphereAt(glm::vec3(0.0f), 1.0f), sphereAt(glm::vec3(1.5f, 0.0f, 0.0f), 1.0f),
                                     manifold));
    CHECK(manifold.count == 1);
    checkVec(manifold.normal, glm::vec3(1.0f, 0.0f, 0.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.5f));
    checkVec(manifold.points[0].position, glm::vec3(0.75f, 0.0f, 0.0f));

    CHECK_FALSE(cinder::physics::collide(sphereAt(glm::vec3(0.0f), 1.0f),
                                         sphereAt(glm::vec3(2.5f, 0.0f, 0.0f), 1.0f), manifold));
}

TEST_CASE("a sphere resting on a box face touches along the face normal") {
    Manifold manifold;
    const Geometry ball = sphereAt(glm::vec3(0.3f, 0.9f, 0.0f), 0.5f);
    const Geometry floor = boxAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.5f, 2.0f));

    REQUIRE(cinder::physics::collide(ball, floor, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, -1.0f, 0.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.1f));
    checkVec(manifold.points[0].position, glm::vec3(0.3f, 0.5f, 0.0f));

    REQUIRE(cinder::physics::collide(floor, ball, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 1.0f, 0.0f));
}

TEST_CASE("a sphere near a box corner touches along the line to the corner") {
    Manifold manifold;
    const Geometry ball = sphereAt(glm::vec3(1.3f, 1.4f, 0.0f), 0.6f);
    const Geometry box = boxAt(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(cinder::physics::collide(ball, box, manifold));
    checkVec(manifold.normal, -glm::normalize(glm::vec3(0.3f, 0.4f, 0.0f)));
    CHECK(manifold.points[0].depth == doctest::Approx(0.1f));
    checkVec(manifold.points[0].position, glm::vec3(1.0f, 1.0f, 0.0f));
}

TEST_CASE("a sphere whose centre is inside a box leaves through the nearest face") {
    Manifold manifold;
    const Geometry ball = sphereAt(glm::vec3(0.2f, 0.0f, -0.9f), 0.25f);
    const Geometry box = boxAt(glm::vec3(0.0f), glm::vec3(1.0f));

    REQUIRE(cinder::physics::collide(ball, box, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 0.0f, 1.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.35f));
}

TEST_CASE("two boxes meet on the face with the least penetration") {
    Manifold manifold;
    const Geometry below = boxAt(glm::vec3(0.0f), glm::vec3(1.0f));
    const Geometry above = boxAt(glm::vec3(0.0f, 1.8f, 0.0f), glm::vec3(1.0f));

    REQUIRE(cinder::physics::collide(below, above, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 1.0f, 0.0f));
    CHECK(manifold.count == 4);

    std::vector<int> features;
    for (int i = 0; i < manifold.count; ++i) {
        CHECK(manifold.points[static_cast<std::size_t>(i)].depth == doctest::Approx(0.2f));
        CHECK(manifold.points[static_cast<std::size_t>(i)].position.y == doctest::Approx(0.8f));
        features.push_back(manifold.points[static_cast<std::size_t>(i)].feature);
    }
    std::sort(features.begin(), features.end());
    CHECK(std::unique(features.begin(), features.end()) == features.end());
}

TEST_CASE("boxes that clear each other do not touch") {
    Manifold manifold;
    CHECK_FALSE(cinder::physics::collide(boxAt(glm::vec3(0.0f), glm::vec3(1.0f)),
                                         boxAt(glm::vec3(0.0f, 2.01f, 0.0f), glm::vec3(1.0f)),
                                         manifold));
    CHECK_FALSE(cinder::physics::collide(boxAt(glm::vec3(0.0f), glm::vec3(1.0f)),
                                         boxAt(glm::vec3(3.0f, 0.5f, 0.0f), glm::vec3(1.0f)),
                                         manifold));
}

TEST_CASE("a box tipped onto its corner touches at the corner") {
    Manifold manifold;
    const Geometry floor = boxAt(glm::vec3(0.0f), glm::vec3(4.0f, 1.0f, 4.0f));
    Geometry tipped = boxAt(glm::vec3(0.0f, 1.0f + 0.70711f - 0.1f, 0.0f), glm::vec3(0.5f));
    tipped.axes = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::quarter_pi<float>(),
                                        glm::vec3(0.0f, 0.0f, 1.0f)));

    REQUIRE(cinder::physics::collide(floor, tipped, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 1.0f, 0.0f));

    float deepest = 0.0f;
    for (int i = 0; i < manifold.count; ++i) {
        deepest = std::max(deepest, manifold.points[static_cast<std::size_t>(i)].depth);
    }
    CHECK(deepest == doctest::Approx(0.1f).epsilon(0.01));
}

TEST_CASE("written rotations compose back into the orientation they came from") {
    const std::vector<glm::quat> orientations = {
        glm::angleAxis(0.7f, glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f))),
        glm::angleAxis(2.9f, glm::normalize(glm::vec3(-3.0f, 0.5f, 1.0f))),
        glm::angleAxis(glm::half_pi<float>() - 0.001f, glm::vec3(1.0f, 0.0f, 0.0f)),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
    };

    NodeTypes types;
    Scene scene{types};
    cinder::components::registerBuiltins(types);
    Node* node = scene.create<cinder::components::Group>(nullptr);

    for (const glm::quat& orientation : orientations) {
        const glm::vec3 rotation = cinder::physics::rotationOf(orientation);
        node->transform()->setRotation(rotation.x, rotation.y, rotation.z);
        const glm::mat3 expected = glm::mat3_cast(orientation);
        const glm::mat3 actual(node->transform()->local());
        for (int i = 0; i < 3; ++i) checkVec(actual[i], expected[i], 1e-3);
    }
}

TEST_CASE("a dynamic body falls under gravity with semi-implicit Euler") {
    Fixture f;
    Body* body = f.scene.create<Body>(nullptr);
    body->transform()->setPosition(0.0f, 10.0f, 0.0f);

    f.run(1);
    CHECK(body->velocity().y == doctest::Approx(-9.81f * DT));
    CHECK(body->transform()->position().y == doctest::Approx(10.0f - 9.81f * DT * DT));

    body->setGravityScale(0.0f);
    const float speed = body->velocity().y;
    f.run(1);
    CHECK(body->velocity().y == doctest::Approx(speed));
}

TEST_CASE("physics never moves a scene that is not stepped") {
    Fixture f;
    Body* body = f.ball(0.0f, 5.0f, 0.0f);
    for (int i = 0; i < 10; ++i) f.scene.update(DT);
    checkVec(worldOf(body), glm::vec3(0.0f, 5.0f, 0.0f));
}

TEST_CASE("a sphere comes to rest on a collider with no body") {
    Fixture f;
    f.floor();
    Body* body = f.ball(0.0f, 3.0f, 0.0f);

    f.run(300);
    CHECK(worldOf(body).y == doctest::Approx(0.5f).epsilon(0.01));
    CHECK(glm::length(body->velocity()) < 0.05f);
    CHECK(worldOf(body).x == 0.0f);
    CHECK(worldOf(body).z == 0.0f);
}

TEST_CASE("a static body is a floor too, and a kinematic one ignores gravity") {
    Fixture f;
    Body* ground = f.scene.create<Body>(nullptr);
    ground->setMotion(Body::Motion::Static);
    f.floor(ground);
    Body* lift = f.ball(3.0f, 2.0f, 0.0f);
    lift->setMotion(Body::Motion::Kinematic);
    Body* body = f.ball(0.0f, 3.0f, 0.0f);

    f.run(300);
    CHECK(worldOf(body).y == doctest::Approx(0.5f).epsilon(0.01));
    checkVec(worldOf(ground), glm::vec3(0.0f));
    checkVec(worldOf(lift), glm::vec3(3.0f, 2.0f, 0.0f));
}

TEST_CASE("restitution bounces a falling sphere back up") {
    Fixture f;
    f.floor();
    Body* body = f.ball(0.0f, 5.5f, 0.0f);
    static_cast<Collider*>(body->children().front())->setRestitution(0.8f);

    float peak = 0.0f;
    bool bounced = false;
    for (int i = 0; i < 180; ++i) {
        f.run(1);
        if (body->velocity().y > 0.0f) bounced = true;
        if (bounced) peak = std::max(peak, worldOf(body).y);
    }

    REQUIRE(bounced);
    const float ratio = (peak - 0.5f) / 5.0f;
    CHECK(ratio > 0.5f);
    CHECK(ratio < 0.8f);
}

TEST_CASE("equal spheres swap velocities in an elastic head-on collision") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* left = f.ball(-2.0f, 0.0f, 0.0f);
    Body* right = f.ball(2.0f, 0.0f, 0.0f);
    left->setVelocity(3.0f, 0.0f, 0.0f);
    right->setVelocity(-1.0f, 0.0f, 0.0f);
    static_cast<Collider*>(left->children().front())->setRestitution(1.0f);

    f.run(120);
    CHECK(left->velocity().x == doctest::Approx(-1.0f).epsilon(0.02));
    CHECK(right->velocity().x == doctest::Approx(3.0f).epsilon(0.02));
    CHECK(left->velocity().x + right->velocity().x == doctest::Approx(2.0f).epsilon(1e-4));
    CHECK(glm::length(left->angularVelocity()) < 1e-4f);
}

TEST_CASE("a kinematic body pushes a dynamic one and takes its velocity from its motion") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* pusher = f.ball(-2.0f, 0.0f, 0.0f);
    pusher->setMotion(Body::Motion::Kinematic);
    Body* target = f.ball(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 60; ++i) {
        pusher->transform()->translate(2.0f * DT, 0.0f, 0.0f);
        f.run(1);
    }

    CHECK(pusher->velocity().x == doctest::Approx(2.0f).epsilon(1e-3));
    CHECK(worldOf(pusher).x == doctest::Approx(0.0f).epsilon(1e-3));
    CHECK(target->velocity().x > 1.5f);
    CHECK(worldOf(target).x > worldOf(pusher).x + 0.9f);
}

TEST_CASE("writing a body's position teleports it and keeps its velocity") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* body = f.scene.create<Body>(nullptr);
    body->setVelocity(1.0f, 0.0f, 0.0f);

    f.run(10);
    body->transform()->setPosition(0.0f, 7.0f, 0.0f);
    f.run(1);
    checkVec(worldOf(body), glm::vec3(DT, 7.0f, 0.0f));
    CHECK(body->velocity().x == 1.0f);
}

TEST_CASE("moving a body's parent carries the body with it") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Node* group = f.scene.create<cinder::components::Group>(nullptr);
    Body* body = f.scene.create<Body>(group);
    body->transform()->setPosition(0.0f, 5.0f, 0.0f);
    body->setVelocity(1.0f, 0.0f, 0.0f);

    f.run(1);
    group->transform()->setPosition(10.0f, 0.0f, 0.0f);
    f.run(1);
    checkVec(worldOf(body), glm::vec3(10.0f + 2.0f * DT, 5.0f, 0.0f));
}

TEST_CASE("a body under a rotated parent keeps its world pose when written back") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Node* group = f.scene.create<cinder::components::Group>(nullptr);
    group->transform()->setPosition(1.0f, 2.0f, 3.0f).setRotation(0.4f, 1.1f, -0.3f);
    Body* body = f.scene.create<Body>(group);

    f.run(1);
    const glm::vec3 before = worldOf(body);
    body->setVelocity(0.0f, 1.0f, 0.0f);
    f.run(60);
    checkVec(worldOf(body), before + glm::vec3(0.0f, 1.0f, 0.0f), 1e-3);
}

TEST_CASE("a spinning body turns about its centre of mass") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* body = f.scene.create<Body>(nullptr);
    Collider* collider = f.scene.create<Collider>(body);
    collider->setShape(Shape::Sphere);
    collider->transform()->setPosition(1.0f, 0.0f, 0.0f);
    body->setAngularVelocity(0.0f, glm::pi<float>(), 0.0f);

    f.run(60);
    checkVec(worldOf(collider), glm::vec3(1.0f, 0.0f, 0.0f), 1e-3);
    checkVec(worldOf(body), glm::vec3(2.0f, 0.0f, 0.0f), 1e-3);
}

TEST_CASE("a body two spheres wide rests level on the floor") {
    Fixture f;
    f.floor();
    Body* body = f.scene.create<Body>(nullptr);
    body->transform()->setPosition(0.0f, 2.0f, 0.0f);
    for (const float x : {-1.0f, 1.0f}) {
        Collider* collider = f.scene.create<Collider>(body);
        collider->setShape(Shape::Sphere);
        collider->transform()->setPosition(x, 0.0f, 0.0f);
    }

    f.run(300);
    CHECK(worldOf(body).y == doctest::Approx(0.5f).epsilon(0.01));
    CHECK(std::abs(body->transform()->rotation().z) < 5e-3f);
}

TEST_CASE("disabled bodies and colliders take no part in the step") {
    Fixture f;
    Collider* floor = f.floor();
    Body* body = f.ball(0.0f, 3.0f, 0.0f);
    Node* group = f.scene.create<cinder::components::Group>(nullptr);
    Body* hidden = f.ball(5.0f, 3.0f, 0.0f, group);
    group->setEnabled(false);
    floor->setEnabled(false);

    f.run(60);
    CHECK(worldOf(body).y < 0.0f);
    checkVec(worldOf(hidden), glm::vec3(5.0f, 3.0f, 0.0f));
}

TEST_CASE("a destroyed body leaves the world") {
    Fixture f;
    f.floor();
    Body* body = f.ball(0.0f, 3.0f, 0.0f);
    Body* other = f.ball(0.0f, 1.0f, 0.0f);
    f.run(5);
    other->destroy();
    f.run(300);
    CHECK(worldOf(body).y == doctest::Approx(0.5f).epsilon(0.01));
}

TEST_CASE("the same scene steps to the same bits") {
    auto simulate = [] {
        Fixture f;
        f.floor();
        for (int i = 0; i < 6; ++i) {
            f.ball(0.3f * static_cast<float>(i % 3), 1.0f + 1.1f * static_cast<float>(i), 0.2f * static_cast<float>(i % 2));
        }
        f.run(240);
        std::vector<float> out;
        for (Node* root : f.scene.roots()) {
            const glm::vec3 p = root->transform()->position();
            const glm::vec3 r = root->transform()->rotation();
            out.insert(out.end(), {p.x, p.y, p.z, r.x, r.y, r.z});
        }
        return out;
    };

    CHECK(simulate() == simulate());
}

TEST_CASE("bodies and colliders round-trip through the scene file") {
    Fixture f;
    Body* body = f.scene.create<Body>(nullptr);
    body->setMotion(Body::Motion::Kinematic).setMass(2.5f).setVelocity(1.0f, 0.0f, 0.0f);
    Collider* collider = f.scene.create<Collider>(body);
    collider->setShape(Shape::Sphere).setSize(2.0f, 2.0f, 2.0f).setRestitution(0.25f);

    Collider* capsule = f.scene.create<Collider>(body);
    capsule->setShape(Shape::Capsule).setSize(1.0f, 2.0f, 1.0f).setFriction(0.25f);

    const std::string text = cinder::serial::SceneCodec::save(f.scene);
    CHECK(text.find("motion \"kinematic\"") != std::string::npos);
    CHECK(text.find("shape \"sphere\"") != std::string::npos);
    CHECK(text.find("shape \"capsule\"") != std::string::npos);

    Fixture g;
    cinder::serial::SceneCodec::load(text, g.scene);
    CHECK(cinder::serial::SceneCodec::save(g.scene) == text);
}

TEST_CASE("a box comes to rest on the floor and stays there") {
    Fixture f;
    f.floor();
    Body* box = f.crate(0.0f, 2.0f, 0.0f);

    f.run(180);
    const glm::vec3 settled = worldOf(box);
    CHECK(settled.y == doctest::Approx(0.5f).epsilon(0.02));

    f.run(300);
    checkVec(worldOf(box), settled, 1e-3);
    CHECK(glm::length(box->velocity()) < 0.01f);
    CHECK(glm::length(box->transform()->rotation()) < 0.01f);
}

TEST_CASE("a stack of ten crates stands up") {
    Fixture f;
    f.floor();

    std::vector<Body*> stack;
    for (int i = 0; i < 10; ++i) stack.push_back(f.crate(0.0f, 0.5f + static_cast<float>(i), 0.0f));

    f.run(600);

    for (std::size_t i = 0; i < stack.size(); ++i) {
        CAPTURE(i);
        const glm::vec3 position = worldOf(stack[i]);
        CHECK(position.y == doctest::Approx(0.5f + static_cast<float>(i)).epsilon(0.05));
        CHECK(std::abs(position.x) < 0.05f);
        CHECK(std::abs(position.z) < 0.05f);
        CHECK(glm::length(stack[i]->transform()->rotation()) < 0.05f);
    }
}

TEST_CASE("friction holds a crate on a slope it cannot slide down") {
    const float tilt = 0.349f;

    auto slide = [tilt](float friction) {
        Fixture f;
        Collider* ramp = f.scene.create<Collider>(nullptr);
        ramp->setSize(20.0f, 1.0f, 20.0f).setFriction(friction);
        ramp->transform()->setRotation(0.0f, 0.0f, tilt);

        const glm::vec3 up(-std::sin(tilt), std::cos(tilt), 0.0f);
        const glm::vec3 seat = up * 1.0f;
        Body* box = f.scene.create<Body>(nullptr);
        box->transform()->setPosition(seat.x, seat.y, seat.z).setRotation(0.0f, 0.0f, tilt);
        f.scene.create<Collider>(box)->setFriction(friction);

        f.run(180);
        return worldOf(box) - seat;
    };

    const glm::vec3 held = slide(0.8f);
    CHECK(glm::length(held) < 0.05f);

    const glm::vec3 slid = slide(0.05f);
    CHECK(slid.x < -1.0f);
    CHECK(slid.y < -0.3f);
}

TEST_CASE("friction brings a sliding crate to a stop") {
    Fixture f;
    f.floor();
    Body* box = f.crate(0.0f, 0.5f, 0.0f);
    box->setVelocity(4.0f, 0.0f, 0.0f);

    f.run(180);
    CHECK(glm::length(box->velocity()) < 0.05f);
    CHECK(worldOf(box).x > 1.0f);
    CHECK(worldOf(box).x < 2.5f);
}

TEST_CASE("friction turns a sliding sphere into a rolling one") {
    Fixture f;
    f.floor();
    Body* ball = f.ball(0.0f, 0.5f, 0.0f);
    ball->setVelocity(4.0f, 0.0f, 0.0f);

    f.run(180);
    CHECK(ball->velocity().x == doctest::Approx(4.0f * 5.0f / 7.0f).epsilon(0.05));
    CHECK(ball->angularVelocity().z == doctest::Approx(-2.0f * ball->velocity().x).epsilon(0.05));
}

TEST_CASE("an impulse changes velocity by impulse over mass") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* body = f.scene.create<Body>(nullptr);
    body->setMass(2.0f);

    body->applyImpulse(glm::vec3(4.0f, 0.0f, 0.0f));
    f.run(1);
    CHECK(body->velocity().x == doctest::Approx(2.0f));

    f.run(1);
    CHECK(body->velocity().x == doctest::Approx(2.0f));
}

TEST_CASE("an impulse off the centre of mass adds spin") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* ball = f.ball(0.0f, 0.0f, 0.0f);

    ball->applyImpulse(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.5f, 0.0f, 0.0f));
    f.run(1);

    CHECK(ball->velocity().z == doctest::Approx(-1.0f));
    CHECK(ball->angularVelocity().y == doctest::Approx(5.0f));
}

TEST_CASE("a force lasts one step") {
    Fixture f;
    f.world.setGravity(glm::vec3(0.0f));
    Body* body = f.scene.create<Body>(nullptr);

    body->applyForce(glm::vec3(3.0f, 0.0f, 0.0f));
    f.run(1);
    const float gained = body->velocity().x;
    CHECK(gained == doctest::Approx(3.0f * DT));

    f.run(1);
    CHECK(body->velocity().x == doctest::Approx(gained));
}

TEST_CASE("a planar body keeps its depth and turns only about z") {
    Fixture f;
    f.floor();
    Body* box = f.crate(0.0f, 3.0f, 2.0f);
    box->setPlanar(true).setVelocity(1.0f, 0.0f, 5.0f).setAngularVelocity(2.0f, 2.0f, 2.0f);

    f.run(120);
    CHECK(worldOf(box).z == 2.0f);
    CHECK(box->velocity().z == 0.0f);
    CHECK(box->transform()->rotation().x == 0.0f);
    CHECK(box->transform()->rotation().y == 0.0f);
    CHECK(std::abs(box->transform()->rotation().z) > 0.1f);
}

TEST_CASE("a ray reports the nearest shape it meets") {
    Fixture f;
    Collider* floor = f.floor();
    Body* ball = f.ball(0.0f, 3.0f, 0.0f);

    cinder::physics::RayHit hit;
    REQUIRE(f.world.raycast(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 100.0f, hit));
    CHECK(hit.node == ball);
    CHECK(hit.distance == doctest::Approx(6.5f));
    checkVec(hit.position, glm::vec3(0.0f, 3.5f, 0.0f));
    checkVec(hit.normal, glm::vec3(0.0f, 1.0f, 0.0f));

    CHECK_FALSE(f.world.raycast(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 100.0f, hit));
    CHECK_FALSE(f.world.raycast(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 5.0f, hit));

    ball->setEnabled(false);
    REQUIRE(f.world.raycast(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 100.0f, hit));
    CHECK(hit.node == floor);
    CHECK(hit.distance == doctest::Approx(10.0f));
}

TEST_CASE("touches are announced when they begin and when they end") {
    struct Listener final : cinder::physics::ContactObserver {
        std::vector<std::pair<int, int>> began;
        std::vector<std::pair<int, int>> ended;

        void touched(Node& a, Node& b) override { began.push_back({a.id(), b.id()}); }
        void touchEnded(Node& a, Node& b) override { ended.push_back({a.id(), b.id()}); }
    };

    Fixture f;
    Listener listener;
    f.world.setObserver(&listener);

    Collider* floor = f.floor();
    Body* ball = f.ball(0.0f, 3.0f, 0.0f);

    f.run(120);
    REQUIRE(listener.began.size() == 1);
    CHECK(listener.began[0].first == floor->id());
    CHECK(listener.began[0].second == ball->id());
    CHECK(listener.ended.empty());

    ball->transform()->setPosition(0.0f, 8.0f, 0.0f);
    f.run(1);
    REQUIRE(listener.ended.size() == 1);
    CHECK(listener.ended[0].second == ball->id());

    f.run(150);
    CHECK(listener.began.size() == 2);
}

TEST_CASE("a capsule is a segment of its size with round ends") {
    const Geometry capsule =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 3.0f, 1.0f), glm::mat4(1.0f));
    CHECK(capsule.radius == doctest::Approx(0.5f));
    CHECK(cinder::physics::segmentHalf(capsule) == doctest::Approx(1.0f));
    checkVec(cinder::physics::segmentAxis(capsule), glm::vec3(0.0f, 1.0f, 0.0f));

    const Geometry ball =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(2.0f, 1.0f, 2.0f), glm::mat4(1.0f));
    CHECK(ball.radius == doctest::Approx(1.0f));
    CHECK(cinder::physics::segmentHalf(ball) == 0.0f);

    const glm::mat3 inertia = cinder::physics::inertiaOf(capsule, 1.0f);
    CHECK(inertia[1][1] < inertia[0][0]);
    CHECK(inertia[0][0] == doctest::Approx(inertia[2][2]));
}

TEST_CASE("a capsule meets a sphere along its shaft and over its caps") {
    Manifold manifold;
    const Geometry capsule =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 3.0f, 1.0f), glm::mat4(1.0f));

    REQUIRE(cinder::physics::collide(capsule, sphereAt(glm::vec3(0.8f, 0.3f, 0.0f), 0.5f), manifold));
    checkVec(manifold.normal, glm::vec3(1.0f, 0.0f, 0.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.2f));

    REQUIRE(cinder::physics::collide(capsule, sphereAt(glm::vec3(0.0f, 1.9f, 0.0f), 0.5f), manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 1.0f, 0.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.1f));

    REQUIRE(cinder::physics::collide(sphereAt(glm::vec3(0.8f, 0.3f, 0.0f), 0.5f), capsule, manifold));
    checkVec(manifold.normal, glm::vec3(-1.0f, 0.0f, 0.0f));
}

TEST_CASE("a capsule lying on a box touches at both ends") {
    Manifold manifold;
    const glm::mat4 lying = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.9f, 0.0f)),
                                        glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    const Geometry capsule = cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 2.0f, 1.0f), lying);
    const Geometry floor = boxAt(glm::vec3(0.0f), glm::vec3(2.0f, 0.5f, 2.0f));

    REQUIRE(cinder::physics::collide(capsule, floor, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, -1.0f, 0.0f));
    REQUIRE(manifold.count == 2);
    for (int i = 0; i < 2; ++i) {
        const cinder::physics::ContactPoint& point = manifold.points[static_cast<std::size_t>(i)];
        CHECK(point.depth == doctest::Approx(0.1f));
        CHECK(point.position.y == doctest::Approx(0.5f));
        CHECK(std::abs(point.position.x) == doctest::Approx(0.5f));
    }
    CHECK(manifold.points[0].feature != manifold.points[1].feature);
}

TEST_CASE("a capsule standing on a box touches once") {
    Manifold manifold;
    const glm::mat4 standing = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.4f, 0.0f));
    const Geometry capsule =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 2.0f, 1.0f), standing);

    REQUIRE(cinder::physics::collide(capsule, boxAt(glm::vec3(0.0f), glm::vec3(2.0f, 0.5f, 2.0f)),
                                     manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, -1.0f, 0.0f));
    CHECK(manifold.count == 1);
    CHECK(manifold.points[0].depth == doctest::Approx(0.1f));
}

TEST_CASE("crossed capsules meet where they are closest") {
    Manifold manifold;
    const Geometry upright =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 3.0f, 1.0f), glm::mat4(1.0f));
    const glm::mat4 across = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.9f)),
                                         glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    const Geometry crossing =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 3.0f, 1.0f), across);

    REQUIRE(cinder::physics::collide(upright, crossing, manifold));
    checkVec(manifold.normal, glm::vec3(0.0f, 0.0f, 1.0f));
    CHECK(manifold.points[0].depth == doctest::Approx(0.1f));
}

TEST_CASE("a ray meets a capsule's shaft and its cap") {
    const Geometry capsule =
        cinder::physics::geometryOf(Shape::Capsule, glm::vec3(1.0f, 3.0f, 1.0f), glm::mat4(1.0f));

    float distance = 0.0f;
    glm::vec3 normal(0.0f);
    REQUIRE(cinder::physics::rayHits(capsule, glm::vec3(3.0f, 0.5f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                     distance, normal));
    CHECK(distance == doctest::Approx(2.5f));
    checkVec(normal, glm::vec3(1.0f, 0.0f, 0.0f));

    REQUIRE(cinder::physics::rayHits(capsule, glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                     distance, normal));
    CHECK(distance == doctest::Approx(3.5f));
    checkVec(normal, glm::vec3(0.0f, 1.0f, 0.0f));

    CHECK_FALSE(cinder::physics::rayHits(capsule, glm::vec3(3.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                         distance, normal));
}

TEST_CASE("a capsule lying on the floor settles level") {
    Fixture f;
    f.floor();

    Body* body = f.scene.create<Body>(nullptr);
    body->transform()->setPosition(0.0f, 3.0f, 0.0f).setRotation(0.0f, 0.0f, glm::half_pi<float>());
    f.scene.create<Collider>(body)->setShape(Shape::Capsule).setSize(1.0f, 2.0f, 1.0f);

    f.run(240);
    CHECK(worldOf(body).y == doctest::Approx(0.5f).epsilon(0.02));
    CHECK(body->transform()->rotation().z == doctest::Approx(glm::half_pi<float>()).epsilon(0.02));
    CHECK(glm::length(body->velocity()) < 0.05f);
}

TEST_CASE("the broadphase finds every overlapping pair") {
    std::uint32_t seed = 12345;
    const auto next = [&seed] {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>((seed >> 8) % 2000) / 100.0f;
    };

    std::vector<cinder::physics::Bounds> boxes;
    for (int i = 0; i < 300; ++i) {
        const glm::vec3 centre(next(), next(), next());
        const glm::vec3 half(0.2f + next() * 0.02f, 0.2f + next() * 0.02f, 0.2f + next() * 0.02f);
        boxes.push_back({centre - half, centre + half});
    }

    std::set<std::pair<int, int>> expected;
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        for (std::size_t j = i + 1; j < boxes.size(); ++j) {
            if (cinder::physics::overlaps(boxes[i], boxes[j])) {
                expected.insert({static_cast<int>(i), static_cast<int>(j)});
            }
        }
    }
    REQUIRE(expected.size() > 10);

    cinder::physics::Broadphase tree;
    tree.build(boxes);

    std::set<std::pair<int, int>> found;
    std::vector<int> nearby;
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        tree.query(boxes[i], nearby);
        for (const int index : nearby) {
            const std::size_t j = static_cast<std::size_t>(index);
            if (j <= i || !cinder::physics::overlaps(boxes[i], boxes[j])) continue;
            found.insert({static_cast<int>(i), static_cast<int>(j)});
        }
    }

    CHECK(found == expected);
    CHECK(tree.depth() <= 12);
}

TEST_CASE("a settled stack falls asleep and stops moving at all") {
    Fixture f;
    f.floor();

    std::vector<Body*> stack;
    for (int i = 0; i < 3; ++i) stack.push_back(f.crate(0.0f, 0.5f + static_cast<float>(i), 0.0f));

    f.run(240);
    for (Body* body : stack) {
        CHECK(body->asleep());
        CHECK(body->velocity() == glm::vec3(0.0f));
    }

    std::vector<glm::vec3> settled;
    for (Body* body : stack) settled.push_back(body->transform()->position());

    f.run(600);
    for (std::size_t i = 0; i < stack.size(); ++i) {
        CHECK(stack[i]->transform()->position() == settled[i]);
    }
}

TEST_CASE("a falling ball wakes the stack it lands on") {
    Fixture f;
    f.floor();
    Body* crate = f.crate(0.0f, 0.5f, 0.0f);

    f.run(240);
    REQUIRE(crate->asleep());

    Body* ball = f.ball(0.2f, 4.0f, 0.0f);
    f.run(50);
    CHECK_FALSE(crate->asleep());
    CHECK_FALSE(ball->asleep());

    f.run(300);
    CHECK(crate->asleep());
    CHECK(ball->asleep());
    CHECK(worldOf(ball).y == doctest::Approx(1.5f).epsilon(0.03));
}

TEST_CASE("moving or pushing a sleeping body wakes it") {
    Fixture f;
    f.floor();
    Body* moved = f.crate(0.0f, 0.5f, 0.0f);
    Body* pushed = f.crate(4.0f, 0.5f, 0.0f);
    Body* written = f.crate(8.0f, 0.5f, 0.0f);

    f.run(240);
    REQUIRE(moved->asleep());
    REQUIRE(pushed->asleep());
    REQUIRE(written->asleep());

    moved->transform()->setPosition(0.0f, 4.0f, 0.0f);
    pushed->applyImpulse(glm::vec3(0.0f, 5.0f, 0.0f));
    written->setVelocity(2.0f, 0.0f, 0.0f);

    f.run(2);
    CHECK_FALSE(moved->asleep());
    CHECK_FALSE(pushed->asleep());
    CHECK_FALSE(written->asleep());
    CHECK(pushed->velocity().y > 1.0f);
    CHECK(written->velocity().x > 0.5f);
}

TEST_CASE("a sleeping body wakes when the floor under it goes away") {
    Fixture f;
    Collider* floor = f.floor();
    Body* crate = f.crate(0.0f, 0.5f, 0.0f);

    f.run(240);
    REQUIRE(crate->asleep());

    floor->setEnabled(false);
    f.run(30);
    CHECK_FALSE(crate->asleep());
    CHECK(worldOf(crate).y < 0.4f);
}
