#pragma once

#include "ui/core/Reply.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace cinder::ui {

enum class TextCommit : std::uint8_t { Enter, FocusLost, Cleared };

using OnClicked = std::function<Reply()>;
using OnBoolChanged = std::function<void(bool)>;
using OnFloatChanged = std::function<void(float)>;
using OnTextChanged = std::function<void(const std::string&)>;
using OnTextCommitted = std::function<void(const std::string&, TextCommit)>;
using OnVoid = std::function<void()>;

}
