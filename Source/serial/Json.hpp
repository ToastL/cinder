#pragma once

#include "scene/PropValue.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::serial {

cinder::scene::PropValue parseJson(std::string_view source);

class JsonWriter {
public:
    const std::string& text() const { return out_; }

    void beginObject(std::string_view key = {});
    void beginArray(std::string_view key = {});
    void end();

    void text(std::string_view key, std::string_view value);
    void integer(std::string_view key, std::int64_t value);
    void flag(std::string_view key, bool value);
    void item(std::string_view value);

private:
    struct Level {
        bool array;
        bool empty;
    };

    void next(std::string_view key);
    void quote(std::string_view value);

    std::string out_;
    std::vector<Level> stack_;
};

}
