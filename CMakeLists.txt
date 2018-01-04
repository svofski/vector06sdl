cmake_minimum_required(VERSION 2.8.12)

PROJECT(v06x)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release Debug NoConfig)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	message("This is Windows. Assuming MinGW from https://nuwen.net/mingw.html")
	set(EXTRA_LIBS -limm32 -lwinmm -lversion)
	set(BFDNAME -O pe-x86-64 -B i386:x86-64)
else()
	message("Assuming this is Linux on Raspberry Pi, but will get smarter later")
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		set(BFDNAME -O elf32-littlearm -B arm)
		add_definitions(-mfpu=neon-vfpv4 -mfloat-abi=hard)
	else()
		message("You need to figure out -O and -B options for objcopy")
	endif()	
endif()


set(SOURCE_FILES main.cpp board.cpp i8080.cpp options.cpp)
add_executable(v06x ${SOURCE_FILES} boots.o)

find_package(Boost COMPONENTS thread system filesystem program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

target_link_libraries(v06x ${SDL2_LIBRARY} ${Boost_LIBRARIES})

find_program(OBJCOPY objcopy)
message("objcopy is ${OBJCOPY}")

# Have boots.bin copied first to avoid using long paths
# objcopy generates absurd names if paths are used
set(BOOT_BIN ${CMAKE_CURRENT_SOURCE_DIR}/boot/boots.bin)
file(COPY ${BOOT_BIN} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
ADD_CUSTOM_COMMAND(
    OUTPUT boots.o
    COMMAND objcopy -I binary ${BFDNAME} boots.bin boots.o
    #DEPENDS boots.bin
    )

target_link_libraries(v06x ${BOOT_O} ${EXTRA_LIBS})
