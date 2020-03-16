# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(OBJCOPY ${CMAKE_OBJCOPY})
add_definitions(-DUSED_XXD=1)
set(CMAKE_BUILD_TYPE Release)

#add_compile_options(-save-temps)

message("objcopy is ${OBJCOPY}")

include_directories(../../fast-filters/sources)
include_directories(../../coreutil/sources)
include_directories(../../boost)
include_directories(../../filters)
include_directories(../../src)
include_directories(../../platform/android)
#include_directories(v06x/chaiscript/include)

set(V06X_SOURCE_FILES
        boot/boot.c
        boot/boots.c
        ../../src/options.cpp
        ../../src/memory.cpp
        ../../src/tv.cpp
        ../../src/i8080.cpp
        ../../src/hal.cpp
        ../../src/board.cpp
        ../../src/filler.cpp
        ../../src/cadence.cpp
        ../../src/util.cpp
        ../../src/sound.cpp
        ../../platform/android/emulator_android.cpp
        ../../platform/android/android_main.cpp
        ../../src/resampler.cpp
        )

add_library(v06x-lib SHARED ${V06X_SOURCE_FILES})

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.


target_link_libraries( # Specifies the target library.
        v06x-lib

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})
