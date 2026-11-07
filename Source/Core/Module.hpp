#pragma once

#include "Scene/ClassRegistry.hpp"

#include <string>
#include <utility>
#include <vector>

namespace cinder::Core {

class Registrar {
public:
    explicit Registrar(cinder::Scene::ClassRegistry& types) : types_(types) {}

    template <class T>
    void add(std::string name) {
        if (types_.entry(name) != nullptr) {
            refused_.push_back(std::move(name));
            return;
        }
        types_.add<T>(name);
        added_.push_back(std::move(name));
    }

    const std::vector<std::string>& added() const { return added_; }
    const std::vector<std::string>& refused() const { return refused_; }

private:
    cinder::Scene::ClassRegistry& types_;
    std::vector<std::string> added_;
    std::vector<std::string> refused_;
};

using Enlisted = void (*)(Registrar&);

namespace detail {

__attribute__((visibility("hidden"))) inline std::vector<Enlisted>& enlisted() {
    static std::vector<Enlisted> list;
    return list;
}

__attribute__((visibility("hidden"))) inline bool enlist(Enlisted entry) {
    enlisted().push_back(entry);
    return true;
}

}

}

#define CINDER_REGISTER(Type)                                                              \
    static const bool cinder_enlisted_##Type = ::cinder::Core::detail::enlist(             \
            [](::cinder::Core::Registrar& registrar) { registrar.add<Type>(#Type); })

#define CINDER_MODULE_ENTRY()                                                              \
    extern "C" __attribute__((visibility("default"))) const char* cinder_module_abi() {    \
        return CINDER_MODULE_ABI;                                                          \
    }                                                                                      \
    extern "C" __attribute__((visibility("default"))) void cinder_module_register(         \
            ::cinder::Core::Registrar& registrar) {                                        \
        for (const ::cinder::Core::Enlisted entry : ::cinder::Core::detail::enlisted()) {  \
            entry(registrar);                                                              \
        }                                                                                  \
    }
