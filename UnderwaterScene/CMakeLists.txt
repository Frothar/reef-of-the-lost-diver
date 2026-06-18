cmake_minimum_required(VERSION 3.16)
project(UnderwaterScene LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})  # run from project root so shaders/ models/ textures/ resolve

# ---------------------------------------------------------------------------
# This CMake build targets macOS and Linux. On Windows use UnderwaterScene.sln
# (Visual Studio) with the prebuilt libraries in dependencies/.
#
# macOS prerequisites (Homebrew):
#     brew install glfw glew assimp
# Linux (Debian/Ubuntu):
#     sudo apt install libglfw3-dev libglew-dev libassimp-dev
#
# GLM and ImGui are bundled in dependencies/ and compiled from source, so they
# work identically on every platform.
# ---------------------------------------------------------------------------
if(WIN32)
    message(WARNING "On Windows, prefer the Visual Studio solution (UnderwaterScene.sln). "
                    "This CMake script is configured for macOS/Linux dependencies.")
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)

# --- ImGui (bundled source) ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/dependencies/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)

# --- SOIL (bundled source) ---
file(GLOB SOIL_SOURCES ${CMAKE_SOURCE_DIR}/src/SOIL/*.c)

add_executable(UnderwaterScene
    src/main.cpp
    src/Camera.cpp
    src/Render_Utils.cpp
    src/Shader_Loader.cpp
    src/Texture.cpp
    src/Spline.cpp
    src/FishAnimation.cpp
    src/AnimatedModel.cpp
    ${SOIL_SOURCES}
    ${IMGUI_SOURCES})

target_include_directories(UnderwaterScene PRIVATE
    src
    src/SOIL
    ${CMAKE_SOURCE_DIR}/dependencies/glm     # provides "glm.hpp" / "ext.hpp"
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLEW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}/GL)                 # the code includes "glew.h" unqualified

target_link_libraries(UnderwaterScene PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    assimp::assimp)

# Silence the OpenGL-deprecation warnings on macOS (OpenGL still works up to 4.1).
if(APPLE)
    target_compile_definitions(UnderwaterScene PRIVATE GL_SILENCE_DEPRECATION)
    # SOIL uses CoreFoundation for DXT capability queries on macOS.
    target_link_libraries(UnderwaterScene PRIVATE "-framework CoreFoundation")
endif()
