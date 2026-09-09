#pragma once

#include "reflect/PropType.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace cinder::reflect {

class PropDef;

class PropSink {
public:
    virtual ~PropSink() = default;
    virtual void propChanged(const PropDef& prop) = 0;
};

class PropDef {
public:
    std::string_view name() const { return name_; }
    std::string_view label() const { return label_; }
    PropType type() const { return type_; }
    int arity() const { return arityOf(type_); }
    float min() const { return min_; }
    float max() const { return max_; }
    float step() const { return step_; }

    void read(const void* target, float* out) const;
    void write(void* target, const float* values) const;

    bool readBool(const void* target) const;
    void writeBool(void* target, bool value) const;

    std::string_view readText(const void* target) const;
    void writeText(void* target, std::string_view value) const;

private:
    template <class>
    friend class PropBuilder;

    void notify(void* target) const;

    std::string_view name_;
    std::string label_;
    PropType type_ = PropType::Float;
    float min_ = 0;
    float max_ = 0;
    float step_ = 0;

    void (*readNum_)(const void*, float*) = nullptr;
    void (*writeNum_)(void*, const float*) = nullptr;
    bool (*readFlag_)(const void*) = nullptr;
    void (*writeFlag_)(void*, bool) = nullptr;
    std::string_view (*readStr_)(const void*) = nullptr;
    bool (*writeStr_)(void*, std::string_view) = nullptr;
    PropSink* (*sink_)(void*) = nullptr;
};

using PropList = std::vector<PropDef>;

}
