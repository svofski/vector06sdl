#pragma once
#include "logging.h"

namespace coreutil {

template <class T> FILE *BasicLogSettings<T>::stream = stderr;
template <class T> LogLevel BasicLogSettings<T>::level = LogLevel::Info;

}  // namespace coreutil

#define COREUTIL_LOGAT(l, c, ...) do {             \
    if (::coreutil::LogSettings::level <= (l))     \
      COREUTIL_LOG(c, ##__VA_ARGS__);              \
  } while (0);

#define COREUTIL_LOG(c, fmt, ...) do {                                       \
    std::fprintf(::coreutil::LogSettings::stream, "[%s]", LOG_TAG);          \
    for (ssize_t len = std::strlen(LOG_TAG), nsp = 20 - len; nsp > 0; --nsp) \
      std::fputc(' ', ::coreutil::LogSettings::stream);                      \
    std::fprintf(::coreutil::LogSettings::stream,                            \
                 "%c: " fmt "\n", (c), ##__VA_ARGS__);                       \
  } while (0)
