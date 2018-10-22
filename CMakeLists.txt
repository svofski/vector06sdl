cmake_minimum_required(VERSION 2.8.12)

PROJECT(v06x)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release Debug NoConfig)

message("CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")

add_definitions(-Wall)
add_definitions(-std=gnu++14)
add_definitions(-fpermissive)
add_definitions(-O3)
#add_definitions(-Rpass=loop-vectorize)

include_directories(fast-filters/sources)
include_directories(coreutil/sources)

add_definitions(-ffunction-sections -fdata-sections 
    -Wl,--gc-sections)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message("This is Windows. Assuming MinGW from https://nuwen.net/mingw.html")
    set(EXTRA_LIBS -limm32 -lwinmm -lversion -lws2_32 -lmswsock)
    #target_link_libraries(v06x ws2_32 mswsock)
    set(BFDNAME -O pe-x86-64 -B i386:x86-64)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message("Must be a macOS, will use xxd")
    set(USE_XXD true)
    add_definitions(-ffast-math -msse2 -ffinite-math-only -ftree-vectorize)
else()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        message("Assuming this is Linux on Raspberry Pi")
        set(BFDNAME -O elf32-littlearm -B arm)
        add_definitions(-mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a53 -funsafe-math-optimizations -ftree-vectorize -ftree-vectorizer-verbose=2 -fopt-info-vec)
        #add_definitions(-fopt-info-vec-all)
    else()
        message("Looks like it's a desktop Linux on amd64, will use xxd")
        set(USE_XXD true) 
    endif()	
endif() 

set(SOURCE_FILES src/main.cpp src/memory.cpp src/tv.cpp src/board.cpp src/emulator.cpp
    src/hal.cpp src/i8080.cpp src/options.cpp src/sound.cpp src/filler.cpp
    src/resampler.cpp src/biquad.cpp
    src/cadence.cpp
    src/icon.cpp
    src/glextns.cpp src/shaders.cpp
    src/server.cpp)
add_executable(v06x ${SOURCE_FILES} boots.o singlepass_vsh.o singlepass_fsh.o icon64.o)


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread system filesystem program_options chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
find_package(SDL2_image)
if (${SDL2_image_FOUND}) 
    add_definitions(-DHAS_IMAGE=1)
else()
    message("SDL2_image not found, saving frames will not work")
    set(SDL2_IMAGE_INCLUDE_DIRS "")
    set(SDL2_IMAGE_LIBRARIES "")
endif()
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIRS})

find_package(OpenGL)
message("OPENGL_FOUND: ${OPENGL_FOUND}")
if (${OPENGL_FOUND})
    add_definitions(-DHAVE_OPENGL=1)
endif()

target_link_libraries(v06x ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${Boost_LIBRARIES} ${OPENGL_LIBRARIES})

get_directory_property(include_dirs INCLUDE_DIRECTORIES)
find_path(PERFTOOLS_H gperftools/profiler.h ${include_dirs})
if (PERFTOOLS_H)
    message("Found gperftools/profiler.h")
    add_definitions(-DHAVE_GPERFTOOLS=1)
    set(EXTRA_LIBS ${EXTRA_LIBS} -lprofiler)
endif()

# On Linux and Windows we have objcopy...
find_program(OBJCOPY objcopy)

macro(BIN2OBJ infile basename)
    if(USE_XXD OR (${OBJCOPY} STREQUAL "OBJCOPY-NOTFOUND")) 
        # ...but Darwin is retarded so we must make do with something else
        add_custom_command(OUTPUT ${basename}.c COMMAND xxd -i ${infile} ${basename}.c)
        add_custom_command(OUTPUT ${basename}.o COMMAND cc -c ${basename}.c DEPENDS ${basename}.c)
        add_definitions(-DUSED_XXD=1)
    else()
        add_custom_command(
            OUTPUT ${basename}.o
            COMMAND ${OBJCOPY} -I binary ${BFDNAME} ${infile} ${basename}.o
            DEPENDS ${infile}
            )
    endif()
endmacro()

add_custom_command(OUTPUT boots.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/boot/boots.bin ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/boot/boots.bin)

add_custom_command(OUTPUT singlepass.vsh
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/shaders/singlepass.vsh ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/singlepass.vsh )

add_custom_command(OUTPUT singlepass.fsh
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/shaders/singlepass.fsh ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/singlepass.fsh)

add_custom_command(OUTPUT icon64.rgba
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/res/icon64.rgba ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/icon64.rgba)

    

BIN2OBJ(boots.bin boots)
BIN2OBJ(singlepass.vsh singlepass_vsh)
BIN2OBJ(singlepass.fsh singlepass_fsh)
BIN2OBJ(icon64.rgba icon64)

message("objcopy is ${OBJCOPY}")

target_link_libraries(v06x ${BOOT_O} ${EXTRA_LIBS})

set(TEST_SOURCE_FILES test/tests.cpp)
include_directories(src)
add_executable(tests ${TEST_SOURCE_FILES} boots.o)

