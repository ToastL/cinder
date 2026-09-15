#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <type_traits>

namespace cinder::reflect {

enum class PropType { Float, Int, Vec2, Vec3, Vec4, Bool, String, Enum };

enum class PropHint { None, Color, Angle };

constexpr int arityOf(PropType type) {
    switch (type) {
        case PropType::Float:
        case PropType::Int: return 1;
        case PropType::Vec2: return 2;
        case PropType::Vec3: return 3;
        case PropType::Vec4: return 4;
        default: return 0;
    }
}

constexpr bool inPlace(PropType type) {
    return type == PropType::Vec2 || type == PropType::Vec3 || type == PropType::Vec4;
}

template <class T, class = void>
struct PropTypeOf {};

template <> struct PropTypeOf<float>       { static constexpr PropType value = PropType::Float; };
template <> struct PropTypeOf<int>         { static constexpr PropType value = PropType::Int; };
template <> struct PropTypeOf<bool>        { static constexpr PropType value = PropType::Bool; };
template <> struct PropTypeOf<std::string> { static constexpr PropType value = PropType::String; };
template <> struct PropTypeOf<glm::vec2>   { static constexpr PropType value = PropType::Vec2; };
template <> struct PropTypeOf<glm::vec3>   { static constexpr PropType value = PropType::Vec3; };
template <> struct PropTypeOf<glm::vec4>   { static constexpr PropType value = PropType::Vec4; };

template <class T>
struct PropTypeOf<T, std::enable_if_t<std::is_enum_v<T>>> {
    static constexpr PropType value = PropType::Enum;
};

template <class T, class = void>
struct IsPropType : std::false_type {};

template <class T>
struct IsPropType<T, std::void_t<decltype(PropTypeOf<T>::value)>> : std::true_type {};

}
