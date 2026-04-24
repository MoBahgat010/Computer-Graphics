cmake_minimum_required(VERSION 3.5)     # CMake 4+ removed compatibility with versions older than 3.5
project(GFX-LAB VERSION 0.1.0)          # Here we select the project name and version

# Linux compatibility
if(UNIX AND NOT APPLE)
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
endif()

# Required for loading non-OBJ meshes (e.g., FBX)
find_package(assimp CONFIG QUIET)

# MSYS2 packages may provide assimp headers/libs without assimpConfig.cmake.
if(NOT TARGET assimp::assimp)
        find_path(ASSIMP_INCLUDE_DIRS
                NAMES assimp/Importer.hpp
                PATHS
                        C:/msys64/mingw64/include
                        C:/msys64/ucrt64/include
                        C:/msys64/clang64/include
        )

        find_library(ASSIMP_LIBRARIES
                NAMES assimp
                PATHS
                        C:/msys64/mingw64/lib
                        C:/msys64/ucrt64/lib
                        C:/msys64/clang64/lib
        )

        if(NOT ASSIMP_INCLUDE_DIRS OR NOT ASSIMP_LIBRARIES)
                message(FATAL_ERROR "Assimp was not found. Install it (e.g., MSYS2 package mingw-w64-x86_64-assimp) or set assimp_DIR/ASSIMP_INCLUDE_DIRS/ASSIMP_LIBRARIES.")
        endif()
endif()

# Here we select C++17 with all the standards required and all compiler-specific extensions disabled
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# These are the options we select for building GLFW as a library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)        # Don't build Documentation
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)       # Don't build Tests
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)    # Don't build Examples
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)           # Don't build Installation Information
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)     # Add variables to use High Performance Graphics Card if available
add_subdirectory(vendor/glfw)                       # Build the GLFW project to use later as a library

# A variable with all the source files of GLAD
set(GLAD_SOURCE vendor/glad/src/gl.c)
# A variables with all the source files of Dear ImGui
set(IMGUI_SOURCES
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/imgui_impl/imgui_impl_glfw.cpp
        vendor/imgui/imgui_impl/imgui_impl_opengl3.cpp
        )

# Combine all vendor source files together into a single variable
set(VENDOR_SOURCES ${GLAD_SOURCE} ${IMGUI_SOURCES})

# A variable with all our source files that are common between executable targets (examples)
set(COMMON_SOURCES
        source/common/application.hpp
        source/common/application.cpp
        source/common/audio/audio-player.hpp
        source/common/audio/audio-player.cpp
        source/common/input/keyboard.hpp
        source/common/input/mouse.hpp

        source/common/components/PlayerComponents/player-component.hpp
source/common/components/PlayerComponents/player-component.cpp

        source/common/asset-loader.cpp
        source/common/asset-loader.hpp
        source/common/deserialize-utils.hpp

        source/common/shader/shader.hpp
        source/common/shader/shader.cpp

        source/common/mesh/VAO/VAO.cpp
        source/common/mesh/VBO/VBO.cpp
        source/common/mesh/EBO/EBO.cpp

        source/common/mesh/vertex.hpp
        source/common/mesh/mesh.hpp
        source/common/mesh/mesh-utils.hpp
        source/common/mesh/mesh-utils.cpp

        source/common/texture/sampler.hpp
        source/common/texture/sampler.cpp
        source/common/texture/texture2d.hpp
        source/common/texture/texture-utils.hpp
        source/common/texture/texture-utils.cpp
        source/common/texture/screenshot.hpp
        source/common/texture/screenshot.cpp

        source/common/material/pipeline-state.hpp
        source/common/material/pipeline-state.cpp
        source/common/material/material.hpp
        source/common/material/material.cpp

        source/common/ecs/component.hpp
        source/common/ecs/transform.hpp
        source/common/ecs/transform.cpp
        source/common/ecs/entity.hpp
        source/common/ecs/entity.cpp
        source/common/ecs/world.hpp
        source/common/ecs/world.cpp

        source/common/components/camera.hpp
        source/common/components/camera.cpp
        source/common/components/mesh-renderer.hpp
        source/common/components/mesh-renderer.cpp
        source/common/components/free-camera-controller.hpp
        source/common/components/free-camera-controller.cpp
        source/common/components/movement.hpp
        source/common/components/movement.cpp
        source/common/components/animation-component.hpp
        source/common/components/animation-component.cpp
        source/common/components/PlayerComponents/player-component.hpp
        source/common/components/PlayerComponents/player-component.cpp
        source/common/components/EnemyComponents/enemy-soldier-component.hpp
        source/common/components/EnemyComponents/enemy-soldier-component.cpp
        source/common/components/component-deserializer.hpp

        source/common/animation/animated-mesh.hpp
        source/common/animation/animation.hpp
        source/common/animation/animation.cpp
        source/common/animation/animator.hpp
        source/common/animation/animator.cpp
        source/common/animation/bone.hpp
        source/common/animation/bone.cpp

        source/common/systems/forward-renderer.hpp
        source/common/systems/forward-renderer.cpp
        source/common/systems/free-camera-controller.hpp
        source/common/systems/movement.hpp
        source/common/systems/PlayerController/player-controller.hpp
        source/common/systems/PlayerController/player-controller.cpp
        source/common/systems/EnemyController/enemy-soldier-controller.hpp
        source/common/systems/EnemyController/enemy-soldier-controller.cpp
)

# Define the directories in which to search for the included headers
include_directories(
        source/common
        vendor/glfw/include
        vendor/glad/include
        vendor/glm
        vendor/imgui
        vendor/miniaudio
        vendor/utils
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)

set(STATES_SOURCES
        source/states/play-state.hpp
        source/states/menu-state.hpp
        source/states/shader-test-state.hpp
        source/states/mesh-test-state.hpp
        source/states/transform-test-state.hpp
        source/states/pipeline-test-state.hpp
        source/states/texture-test-state.hpp
        source/states/sampler-test-state.hpp
        source/states/material-test-state.hpp
        source/states/entity-test-state.hpp
        source/states/renderer-test-state.hpp
)

# For each example, we add an executable target
# Each target compiles one example source file and the common & vendor source files
# Then we link GLFW with each target
add_executable(GAME_APPLICATION source/main.cpp ${STATES_SOURCES} ${COMMON_SOURCES} ${VENDOR_SOURCES})
target_link_libraries(GAME_APPLICATION glfw)

if(WIN32)
        target_link_libraries(GAME_APPLICATION winmm ole32 avrt uuid)
endif()

if(TARGET assimp::assimp)
        target_link_libraries(GAME_APPLICATION assimp::assimp)
else()
        target_include_directories(GAME_APPLICATION PRIVATE ${ASSIMP_INCLUDE_DIRS})
        target_link_libraries(GAME_APPLICATION ${ASSIMP_LIBRARIES})
endif()

target_compile_definitions(GAME_APPLICATION PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)

if(UNIX AND NOT APPLE)
        target_link_libraries(GAME_APPLICATION OpenGL::GL)
        target_link_libraries(GAME_APPLICATION GLEW::GLEW)
endif()