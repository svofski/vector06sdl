cmake_minimum_required(VERSION 2.8.12)

PROJECT(v06x)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release Debug NoConfig)

set(SOURCE_FILES main.cpp board.cpp i8080.cpp options.cpp)
add_executable(v06x ${SOURCE_FILES} boots.o)

find_package(Boost COMPONENTS thread system filesystem program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

target_link_libraries(v06x ${SDL2_LIBRARY} ${Boost_LIBRARIES})

# Have boots.bin copied first to avoid using long paths
# objcopy generates absurd names if paths are used
set(BOOT_BIN ${CMAKE_CURRENT_SOURCE_DIR}/boot/boots.bin)
ADD_CUSTOM_COMMAND(
    OUTPUT boots.bin
    DEPENDS ${BOOT_BIN}
    COMMAND cp ${BOOT_BIN} boots.bin)
ADD_CUSTOM_COMMAND(
    OUTPUT boots.o
    COMMAND objcopy -I binary -O elf32-littlearm -B arm boots.bin boots.o
    DEPENDS boots.bin
    )

target_link_libraries(v06x ${BOOT_O})
