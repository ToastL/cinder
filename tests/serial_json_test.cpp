#include <doctest/doctest.h>

#include "serial/Json.hpp"

#include <clocale>
#include <cstdint>
#include <stdexcept>
#include <string>

using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;
using cinder::serial::JsonWriter;
using cinder::serial::parseJson;

namespace {

std::string failure(const std::string& source) {
    try {
        parseJson(source);
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    return {};
}

}

TEST_CASE("json parses objects, arrays and scalars") {
    const PropValue document = parseJson(
            R"({ "name": "Box", "count": 3, "scale": -1.5e2, "on": true, "off": false,
                 "tags": ["a", "b"], "empty": {} })");

    REQUIRE(document.is<PropRec>());
    const PropRec& root = document.as<PropRec>();
    CHECK(root.at("name").as<std::string>() == "Box");
    CHECK(root.at("count").as<std::int64_t>() == 3);
    CHECK(root.at("scale").as<double>() == -150.0);
    CHECK(root.at("on").as<bool>());
    CHECK_FALSE(root.at("off").as<bool>());
    REQUIRE(root.at("tags").as<PropSeq>().size() == 2);
    CHECK(root.at("tags").as<PropSeq>()[1].as<std::string>() == "b");
    CHECK(root.at("empty").as<PropRec>().empty());
}

TEST_CASE("json string escapes decode to utf-8") {
    const PropValue document = parseJson(R"(["q\"b\\s\/n\n\t", "\u00e9", "\ud83d\ude00"])");

    const PropSeq& items = document.as<PropSeq>();
    CHECK(items[0].as<std::string>() == "q\"b\\s/n\n\t");
    CHECK(items[1].as<std::string>() == "\xC3\xA9");
    CHECK(items[2].as<std::string>() == "\xF0\x9F\x98\x80");
}

TEST_CASE("malformed json reports its line and column") {
    CHECK(failure("{\n  \"a\": 1,\n}") == "json 3:1: expected a quoted key");
    CHECK(failure("[1, 2") == "json 1:6: expected ',' or ']'");
    CHECK(failure("\"open") == "json 1:6: unterminated string");
    CHECK(failure("null") == "json 1:1: null is not supported");
    CHECK(failure("{} x") == "json 1:4: unexpected text after the document");
    CHECK(failure("\"\\ud83d\"") == "json 1:8: unpaired surrogate");
    CHECK_FALSE(failure(std::string(100, '[')).empty());
}

TEST_CASE("the json writer indents with tabs and keeps empty containers on one line") {
    JsonWriter json;
    json.beginObject();
    json.integer("fileVersion", 1);
    json.text("name", "say \"hi\"\n");
    json.flag("on", true);
    json.beginArray("list");
    json.item("a");
    json.item("b");
    json.end();
    json.beginArray("none");
    json.end();
    json.beginObject("nested");
    json.beginArray("steps");
    json.item("x");
    json.end();
    json.end();
    json.end();

    CHECK(json.text()
          == "{\n\t\"fileVersion\": 1,\n\t\"name\": \"say \\\"hi\\\"\\n\",\n\t\"on\": true,\n"
             "\t\"list\": [\n\t\t\"a\",\n\t\t\"b\"\n\t],\n\t\"none\": [],\n"
             "\t\"nested\": {\n\t\t\"steps\": [\n\t\t\t\"x\"\n\t\t]\n\t}\n}\n");
}

TEST_CASE("json text survives a write and a parse") {
    const std::string tricky = std::string("tab\tquote\"slash\\ctrl") + '\x01' + "\xC3\xA9";

    JsonWriter json;
    json.beginArray();
    json.item(tricky);
    json.end();

    CHECK(json.text() == "[\n\t\"tab\\tquote\\\"slash\\\\ctrl\\u0001\xC3\xA9\"\n]\n");
    CHECK(parseJson(json.text()).as<PropSeq>()[0].as<std::string>() == tricky);
}

TEST_CASE("json numbers do not depend on the default locale") {
    const char* before = std::setlocale(LC_ALL, nullptr);
    const std::string saved = before != nullptr ? before : "C";

    std::setlocale(LC_ALL, "tr_TR.UTF-8");
    const std::string message = failure("[0.5]");
    const PropValue document = message.empty() ? parseJson("[0.5]") : PropValue();
    std::setlocale(LC_ALL, saved.c_str());

    CHECK(message.empty());
    REQUIRE(document.is<PropSeq>());
    CHECK(document.as<PropSeq>()[0].as<double>() == 0.5);
}
