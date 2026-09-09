#include <doctest/doctest.h>

#include "reflect/Reflect.hpp"

#include <clocale>
#include <string>
#include <vector>

enum class Mode { Low, High };

CINDER_ENUM_NAMES(Mode, "low", "high")

namespace {

struct Fixture : cinder::reflect::PropSink {
    int count = 1;
    float fov = 60;
    bool on = true;
    std::string label = "a";
    Mode mode = Mode::Low;
    glm::vec2 size{2, 3};
    glm::vec3 offset{1, 2, 3};

    int changes = 0;

    void propChanged(const cinder::reflect::PropDef&) override { changes++; }

    CINDER_PROPS(Fixture, void) {
        CINDER_PROP(count);
        CINDER_PROP_R(fov, 1.0f, 179.0f);
        CINDER_PROP(on);
        CINDER_PROP(label);
        CINDER_PROP(mode);
        CINDER_PROP(size);
        CINDER_PROP(offset);
    }
};

const cinder::reflect::PropDef& prop(std::string_view name) {
    for (const cinder::reflect::PropDef& def : cinder::reflect::props<Fixture>()) {
        if (def.name() == name) return def;
    }
    FAIL("no such prop: " << name);
    return cinder::reflect::props<Fixture>().front();
}

}

TEST_CASE("arity matches the declared type") {
    CHECK(prop("fov").arity() == 1);
    CHECK(prop("size").arity() == 2);
    CHECK(prop("offset").arity() == 3);
    CHECK(prop("label").arity() == 0);
}

TEST_CASE("vector writes copy into the existing instance") {
    Fixture f;
    glm::vec3& held = f.offset;

    const float values[] = {7, 8, 9};
    prop("offset").write(&f, values);

    CHECK(&held == &f.offset);
    CHECK(held.x == doctest::Approx(7));
    CHECK(held.z == doctest::Approx(9));
}

TEST_CASE("writes clamp to the declared range") {
    Fixture f;

    const float high[] = {300};
    prop("fov").write(&f, high);
    CHECK(f.fov == doctest::Approx(179));

    const float low[] = {-10};
    prop("fov").write(&f, low);
    CHECK(f.fov == doctest::Approx(1));
}

TEST_CASE("unbounded props are not clamped") {
    Fixture f;
    const float values[] = {-99999};
    prop("count").write(&f, values);
    CHECK(f.count == -99999);
}

TEST_CASE("enums round trip through their lowercase name") {
    Fixture f;
    CHECK(prop("mode").readText(&f) == "low");

    prop("mode").writeText(&f, "HIGH");
    CHECK(f.mode == Mode::High);
    CHECK(prop("mode").readText(&f) == "high");
}

TEST_CASE("an unknown enum constant changes nothing") {
    Fixture f;
    prop("mode").writeText(&f, "sideways");
    CHECK(f.mode == Mode::Low);
    CHECK(f.changes == 0);
}

TEST_CASE("numeric read round trips") {
    Fixture f;
    float out[4] = {};
    prop("size").read(&f, out);
    CHECK(out[0] == doctest::Approx(2));
    CHECK(out[1] == doctest::Approx(3));
}

TEST_CASE("every write notifies the sink") {
    Fixture f;

    const float values[] = {1, 1, 1};
    prop("offset").write(&f, values);
    prop("on").writeBool(&f, false);
    prop("label").writeText(&f, "b");

    CHECK(f.changes == 3);
}

TEST_CASE("props are in declaration order") {
    CHECK(cinder::reflect::props<Fixture>().front().name() == "count");
}

TEST_CASE("unsupported prop types are rejected at compile time") {
    static_assert(cinder::reflect::IsPropType<float>::value);
    static_assert(cinder::reflect::IsPropType<glm::vec3>::value);
    static_assert(cinder::reflect::IsPropType<Mode>::value);
    static_assert(!cinder::reflect::IsPropType<std::vector<std::string>>::value);
    static_assert(!cinder::reflect::IsPropType<double>::value);
    CHECK(true);
}

TEST_CASE("enum text does not depend on the default locale") {
    Fixture f;
    f.mode = Mode::High;

    const char* before = std::setlocale(LC_ALL, nullptr);
    std::string saved = before != nullptr ? before : "C";

    std::setlocale(LC_ALL, "tr_TR.UTF-8");
    CHECK(prop("mode").readText(&f) == "high");
    std::setlocale(LC_ALL, saved.c_str());
}

TEST_CASE("labels are derived from the field name") {
    CHECK(cinder::reflect::deriveLabel("fov") == "Fov");
    CHECK(cinder::reflect::deriveLabel("maxSpeed") == "Max Speed");
    CHECK(cinder::reflect::deriveLabel("position") == "Position");
}
