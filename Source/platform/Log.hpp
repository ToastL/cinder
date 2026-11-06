#pragma once

#include <functional>
#include <string_view>

namespace cinder::platform {

enum class LogLevel { Info, Error };

using LogSink = std::function<void(LogLevel, std::string_view)>;

void setLogSink(LogSink sink);

void logInfo(const char* format, ...) __attribute__((format(printf, 1, 2)));
void logError(const char* format, ...) __attribute__((format(printf, 1, 2)));

}
