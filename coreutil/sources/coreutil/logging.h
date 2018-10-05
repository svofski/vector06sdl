#pragma once
#include <cstdio>
#include <cstring>

namespace coreutil {

enum class LogLevel { Debug, Info, Warning, Error };

template <class> struct BasicLogSettings {
  static FILE *stream;
  static LogLevel level;
};
typedef BasicLogSettings<void> LogSettings;

}  // namespace coreutil

#define LOGD(...) COREUTIL_LOGAT(::coreutil::LogLevel::Debug, 'D', ##__VA_ARGS__);
#define LOGI(...) COREUTIL_LOGAT(::coreutil::LogLevel::Info, 'I', ##__VA_ARGS__);
#define LOGW(...) COREUTIL_LOGAT(::coreutil::LogLevel::Warning, 'W', ##__VA_ARGS__);
#define LOGE(...) COREUTIL_LOGAT(::coreutil::LogLevel::Error, 'E', ##__VA_ARGS__);

#include "logging.tcc"
