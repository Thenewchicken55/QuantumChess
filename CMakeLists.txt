cmake_minimum_required(VERSION 3.30.0)
project(QuantumChess LANGUAGES CXX)

# --- Build type ---------------------------------------------------------------
# Default to Release; let users override with -DCMAKE_BUILD_TYPE=Debug.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --- C++ standard ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful for clangd / IDE tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Compiler warnings -------------------------------------------------------
# A pragmatic set: catches real bugs (shadowing, virtual dtors, missing
# overrides) without the noise of -Wconversion/-Wsign-conversion/-Wold-style-cast
# which are pervasive when interop-ing with raylib's float/int C API.
add_library(qc_warnings INTERFACE)
if(MSVC)
    target_compile_options(qc_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(qc_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
        -Wmissing-field-initializers
    )
endif()

# --- raylib (pinned to a stable tag, not master) ------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG        "5.5"
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(raylib)

# --- Main executable ----------------------------------------------------------
add_executable(${PROJECT_NAME} "src/main/main.cpp")

# Convenience target: build and run the produced binary, from the source dir so
# the relative "assets/" paths resolve regardless of where the binary lives.
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

file(GLOB MainSources   "src/main/*.cpp")
file(GLOB PieceSources  "src/pieces/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${MainSources} ${PieceSources})

target_include_directories(${PROJECT_NAME} PRIVATE
    include/main
    include/pieces
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib qc_warnings)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# macOS: raylib needs the frameworks linked.
if(APPLE)
    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARY})
endif()

# --- Install ------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY assets DESTINATION share/quantumchess)
