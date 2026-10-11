cmake_minimum_required(VERSION 3.27)

project(cursortrail
    DESCRIPTION "osu!-style glowing cursor trail for Hyprland"
    VERSION 1.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC "src/*.cpp")

add_library(cursortrail SHARED ${SRC})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    libinput
    pixman-1
    wayland-server
    xkbcommon
    egl
    glesv2
)
target_link_libraries(cursortrail PRIVATE rt PkgConfig::deps)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cursortrail PRIVATE --no-gnu-unique)
endif()

install(TARGETS cursortrail)
