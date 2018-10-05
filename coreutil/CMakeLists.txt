cmake_minimum_required(VERSION 3.0)

project(coreutil
  VERSION 0.1
  LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(WarningsEnabled)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(coreutil_SOURCES
  sources/coreutil/cpu.cc
  sources/coreutil/memory.cc
  sources/coreutil/ringbuffer.cc)
add_library(coreutil STATIC ${coreutil_SOURCES})
target_include_directories(coreutil PUBLIC sources)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_compile_definitions(coreutil PUBLIC -D_FILE_OFFSET_BITS=64)
endif()
