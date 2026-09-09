#pragma once

#include "reflect/Enum.hpp"
#include "reflect/PropDef.hpp"
#include "reflect/PropType.hpp"

#include <cfloat>
#include <string>
#include <type_traits>
#include <utility>

namespace cinder::reflect {

std::string deriveLabel(std::string_view field);

template <class Target>
PropSink* sinkOf(void* target) {
    if constexpr (std::is_base_of_v<PropSink, Target>) {
        return static_cast<PropSink*>(static_cast<Target*>(target));
    } else {
        return nullptr;
    }
}

template <class Target>
class PropBuilder {
public:
    explicit PropBuilder(PropList& out) : out_(out) {}

    template <auto Member>
    void add(std::string_view name,
             float min = -FLT_MAX, float max = FLT_MAX, float step = 0.1f) {
        using Ref = decltype(std::declval<Target&>().*Member);
        using Field = std::remove_cvref_t<Ref>;

        static_assert(!std::is_const_v<std::remove_reference_t<Ref>>,
                      "a const prop cannot be written");
        static_assert(IsPropType<Field>::value,
                      "unsupported @Prop type");

        constexpr PropType kind = PropTypeOf<Field>::value;

        if (name.ends_with('_')) name.remove_suffix(1);

        PropDef def;
        def.name_ = name;
        def.label_ = deriveLabel(name);
        def.type_ = kind;
        def.min_ = min;
        def.max_ = max;
        def.step_ = step;
        def.sink_ = &sinkOf<Target>;

        if constexpr (kind == PropType::Float) {
            def.readNum_ = [](const void* p, float* o) {
                o[0] = static_cast<const Target*>(p)->*Member;
            };
            def.writeNum_ = [](void* p, const float* v) {
                static_cast<Target*>(p)->*Member = v[0];
            };
        } else if constexpr (kind == PropType::Int) {
            def.readNum_ = [](const void* p, float* o) {
                o[0] = static_cast<float>(static_cast<const Target*>(p)->*Member);
            };
            def.writeNum_ = [](void* p, const float* v) {
                static_cast<Target*>(p)->*Member = static_cast<int>(v[0]);
            };
        } else if constexpr (inPlace(kind)) {
            def.readNum_ = [](const void* p, float* o) {
                const Field& value = static_cast<const Target*>(p)->*Member;
                for (int i = 0; i < Field::length(); ++i) o[i] = value[i];
            };
            def.writeNum_ = [](void* p, const float* v) {
                Field& value = static_cast<Target*>(p)->*Member;
                for (int i = 0; i < Field::length(); ++i) value[i] = v[i];
            };
        } else if constexpr (kind == PropType::Bool) {
            def.readFlag_ = [](const void* p) { return static_cast<const Target*>(p)->*Member; };
            def.writeFlag_ = [](void* p, bool v) { static_cast<Target*>(p)->*Member = v; };
        } else if constexpr (kind == PropType::String) {
            def.readStr_ = [](const void* p) -> std::string_view {
                return static_cast<const Target*>(p)->*Member;
            };
            def.writeStr_ = [](void* p, std::string_view v) {
                static_cast<Target*>(p)->*Member = std::string(v);
                return true;
            };
        } else {
            def.readStr_ = [](const void* p) -> std::string_view {
                const auto index = static_cast<std::size_t>(static_cast<const Target*>(p)->*Member);
                return index < EnumNames<Field>::count ? EnumNames<Field>::names[index]
                                                       : std::string_view{};
            };
            def.writeStr_ = [](void* p, std::string_view v) {
                for (std::size_t i = 0; i < EnumNames<Field>::count; ++i) {
                    if (!equalsIgnoreAscii(EnumNames<Field>::names[i], v)) continue;
                    static_cast<Target*>(p)->*Member = static_cast<Field>(i);
                    return true;
                }
                return false;
            };
        }

        out_.push_back(std::move(def));
    }

private:
    PropList& out_;
};

template <class T, class Target>
struct PropChain {
    static void run(PropBuilder<Target>& builder) {
        if constexpr (!std::is_void_v<typename T::PropBase>) {
            PropChain<typename T::PropBase, Target>::run(builder);
        }
        T::template declareProps<Target>(builder);
    }
};

template <class T>
const PropList& props() {
    static const PropList cached = [] {
        PropList out;
        PropBuilder<T> builder(out);
        PropChain<T, T>::run(builder);
        return out;
    }();
    return cached;
}

}

#define CINDER_PROPS(Type, Base)                                            \
public:                                                                 \
    using PropSelf = Type;                                              \
    using PropBase = Base;                                              \
    template <class PropTarget>                                         \
    static void declareProps(cinder::reflect::PropBuilder<PropTarget>& b)

#define CINDER_PROP(field)              b.template add<&PropSelf::field>(#field)
#define CINDER_PROP_R(field, lo, hi)    b.template add<&PropSelf::field>(#field, lo, hi)
#define CINDER_PROP_S(field, lo, hi, s) b.template add<&PropSelf::field>(#field, lo, hi, s)
