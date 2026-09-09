#pragma once

#include "scene/PropValue.hpp"

#include <string>
#include <string_view>

namespace cinder::serial {

class Archive {
public:
    virtual ~Archive() = default;

    virtual bool loading() const = 0;

    virtual bool enterRecord(std::string_view name) = 0;
    virtual void leaveRecord() = 0;

    virtual int enterArray(std::string_view name, int count) = 0;
    virtual void enterItem(int index, std::string_view type) = 0;
    virtual std::string itemType(int index) = 0;
    virtual void leaveItem() = 0;
    virtual void leaveArray() = 0;

    virtual int integer(std::string_view name, int value, int fallback) = 0;
    virtual bool flag(std::string_view name, bool value, bool fallback) = 0;
    virtual std::string text(std::string_view name, std::string_view value,
                             std::string_view fallback) = 0;
    virtual void vector(std::string_view name, float* values, const float* fallback, int arity) = 0;

    virtual cinder::scene::PropRec bag(std::string_view name, const cinder::scene::PropRec& values) = 0;
};

}
