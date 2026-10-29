cmake_minimum_required(VERSION 3.20)
project(war_poc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE WAR_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE WAR_MODEL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/model/*.cpp")
list(REMOVE_ITEM WAR_SOURCES ${WAR_MODEL_SOURCES})

function(war_enable_warnings target)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endfunction()

add_library(war_model STATIC ${WAR_MODEL_SOURCES})
target_include_directories(war_model PUBLIC "${CMAKE_SOURCE_DIR}/src")
war_enable_warnings(war_model)

add_executable(war_poc ${WAR_SOURCES})

target_include_directories(war_poc PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(war_poc SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(war_poc PRIVATE war_model)
war_enable_warnings(war_poc)

# The link step runs through cmd.exe, which mangles the non-ASCII absolute repo path,
# so the vendored lib dir is passed relative to the build dir.
file(RELATIVE_PATH WAR_LIB_DIR "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/lib")
target_link_options(war_poc PRIVATE "-L${WAR_LIB_DIR}")
target_link_libraries(war_poc PRIVATE raylib opengl32 gdi32 winmm)

# Static runtime so the executable runs outside the MSYS2 shell.
target_link_options(war_poc PRIVATE -static)

if(EXISTS "${CMAKE_SOURCE_DIR}/resource")
    add_custom_command(TARGET war_poc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resource"
            "$<TARGET_FILE_DIR:war_poc>/resource"
        COMMENT "Copying game resources next to the executable"
        VERBATIM
    )
endif()

install(TARGETS war_poc RUNTIME DESTINATION .)
if(EXISTS "${CMAKE_SOURCE_DIR}/resource")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resource/" DESTINATION resource)
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-windows-x64")
include(CPack)

include(CTest)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Preserve this project's runtime selection when configuring GoogleTest on Windows.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE WAR_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*Tests.cpp")

    add_executable(war_poc_tests ${WAR_TEST_SOURCES})
    target_include_directories(war_poc_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
    )
    target_include_directories(war_poc_tests SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/TextureManager.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/ResourcePath.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/ui/TextCatalog.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/ui/GameText.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/gameplay/TimeHudView.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/AnimatedSpriteViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/StaticSpriteViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/SpriteSheet.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/build/MineResourceViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/build/BuildingSpriteViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/build/FactoryViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/units/DroneViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/units/SoldierSpriteViewModel.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/units/UnitFacing.cpp")
    target_sources(war_poc_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/view/world/ResourceDepositViewModel.cpp")
    target_link_libraries(war_poc_tests PRIVATE war_model GTest::gtest_main)
    war_enable_warnings(war_poc_tests)

    include(GoogleTest)
    gtest_discover_tests(war_poc_tests)
endif()
