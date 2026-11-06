#include "reflect/PropDef.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace cinder::reflect {

void copyProps(const PropList& defs, const void* from, void* to) {
    float values[4]{};
    for (const PropDef& def : defs) {
        switch (def.type()) {
            case PropType::Bool:
                def.writeBool(to, def.readBool(from));
                break;
            case PropType::String:
            case PropType::Enum:
                def.writeText(to, def.readText(from));
                break;
            default:
                def.read(from, values);
                def.write(to, values);
                break;
        }
    }
}

void PropDef::read(const void* target, float* out) const {
    if (readNum_ == nullptr) throw std::runtime_error(std::string(name_) + " is not numeric");
    readNum_(target, out);
}

void PropDef::write(void* target, const float* values) const {
    if (writeNum_ == nullptr) throw std::runtime_error(std::string(name_) + " is not numeric");

    float clamped[4];
    const int count = arity();
    for (int i = 0; i < count; ++i) clamped[i] = std::clamp(values[i], min_, max_);

    writeNum_(target, clamped);
    notify(target);
}

bool PropDef::readBool(const void* target) const { return readFlag_(target); }

void PropDef::writeBool(void* target, bool value) const {
    writeFlag_(target, value);
    notify(target);
}

std::string_view PropDef::readText(const void* target) const { return readStr_(target); }

void PropDef::writeText(void* target, std::string_view value) const {
    if (writeStr_(target, value)) notify(target);
}

void PropDef::notify(void* target) const {
    if (PropSink* sink = sink_(target)) sink->propChanged(*this);
}

}
