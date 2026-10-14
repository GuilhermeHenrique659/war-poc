cmake_minimum_required(VERSION 3.20)
project(war_poc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE WAR_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(war_poc ${WAR_SOURCES})

target_include_directories(war_poc PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(war_poc SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_compile_options(war_poc PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)

# The link step runs through cmd.exe, which mangles the non-ASCII absolute repo path,
# so the vendored lib dir is passed relative to the build dir.
file(RELATIVE_PATH WAR_LIB_DIR "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/lib")
target_link_options(war_poc PRIVATE "-L${WAR_LIB_DIR}")
target_link_libraries(war_poc PRIVATE raylib opengl32 gdi32 winmm)

# Static runtime so the executable runs outside the MSYS2 shell.
target_link_options(war_poc PRIVATE -static)
