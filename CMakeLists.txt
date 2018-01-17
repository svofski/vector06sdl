cmake_minimum_required(VERSION 2.8.12)

PROJECT(v06x)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release Debug NoConfig)

message("CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")

add_definitions(-Wall)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message("This is Windows. Assuming MinGW from https://nuwen.net/mingw.html")
    set(EXTRA_LIBS -limm32 -lwinmm -lversion)
    set(BFDNAME -O pe-x86-64 -B i386:x86-64)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message("Must be a macOS, will use xxd")
    set(USE_XXD true)
else()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        message("Assuming this is Linux on Raspberry Pi")
        set(BFDNAME -O elf32-littlearm -B arm)
        add_definitions(-DZEALOUS_LOCALITY=1)
        add_definitions(-mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a53 -funsafe-math-optimizations -ftree-vectorize -ftree-vectorizer-verbose=2 -fopt-info-vec)
        #add_definitions(-fopt-info-vec-all)
    else()
        message("Looks like it's a desktop Linux on amd64, will use xxd")
        set(USE_XXD true) 
    endif()	
endif() 

set(SOURCE_FILES src/main.cpp src/board.cpp src/i8080.cpp src/options.cpp
    src/biquad.cpp src/cadence.cpp)
add_executable(v06x ${SOURCE_FILES} boots.o)

set_property(TARGET v06x PROPERTY CXX_STANDARD 11)

find_package(Boost COMPONENTS thread system filesystem program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
find_package(SDL2_image)
if (${SDL2_image_FOUND}) 
    add_definitions(-DHAS_IMAGE=1)
else()
    message("SDL2_image not found, saving frames will not work")
    set(SDL2_IMAGE_INCLUDE_DIRS "")
    set(SDL2_IMAGE_LIBRARIES "")
endif()
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIRS})

target_link_libraries(v06x ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${Boost_LIBRARIES})

# Have boots.bin copied first to avoid using long paths
# objcopy generates absurd names if paths are used
set(BOOT_BIN ${CMAKE_CURRENT_SOURCE_DIR}/boot/boots.bin)
file(COPY ${BOOT_BIN} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# On Linux and Windows we have objcopy
find_program(OBJCOPY objcopy)
if(USE_XXD OR (${OBJCOPY} STREQUAL "OBJCOPY-NOTFOUND")) 
    # But Darwin is retarded so we must make do with something else
    add_custom_command(OUTPUT boots.c COMMAND xxd -i boots.bin boots.c)
    add_custom_command(OUTPUT boots.o COMMAND cc -c boots.c DEPENDS boots.c)
    add_definitions(-DUSED_XXD=1)
else()
    add_custom_command(
        OUTPUT boots.o
        COMMAND ${OBJCOPY} -I binary ${BFDNAME} boots.bin boots.o
        #DEPENDS boots.bin
        )

endif()
message("objcopy is ${OBJCOPY}")

target_link_libraries(v06x ${BOOT_O} ${EXTRA_LIBS})

set(TEST_SOURCE_FILES test/tests.cpp)
include_directories(src)
add_executable(tests ${TEST_SOURCE_FILES} boots.o)
