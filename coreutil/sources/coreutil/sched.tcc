#pragma once
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace coreutil {

void sleep_indefinitely() {
#if defined(_WIN32)
  Sleep(INFINITE);
#else
  ::pause();
#endif
}

void sleep_forever() {
  for (;;)
    sleep_indefinitely();
}

}  // namespace coreutil
