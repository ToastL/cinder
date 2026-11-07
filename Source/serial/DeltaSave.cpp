#include "serial/DeltaSave.hpp"

#include "serial/NumberText.hpp"

#include <cstddef>

namespace cinder::serial {

int DeltaSave::integer(std::string_view name, int value, int fallback) {
    if (value != fallback) field(name, integerText(value));
    return value;
}

bool DeltaSave::flag(std::string_view name, bool value, bool fallback) {
    if (value != fallback) field(name, value ? "true" : "false");
    return value;
}

void DeltaSave::vector(std::string_view name, float* values, const float* fallback, int arity) {
    bool same = true;
    for (int i = 0; i < arity; ++i) same = same && values[i] == fallback[i];
    if (same) return;
    field(name, vectorText({values, static_cast<std::size_t>(arity)}));
}

}
