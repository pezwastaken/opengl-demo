cmake_minimum_required(VERSION 3.16)
project(hello)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)


# Create your game executable target as usual
add_executable(hello main.cc mesh.cc glad/src/glad.c imgui/imgui.cpp
imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp
imgui/imgui_widgets.cpp imgui/backends/imgui_impl_sdl3.cpp
imgui/backends/imgui_impl_opengl3.cpp)


target_include_directories(hello PUBLIC glad/include)
target_include_directories(hello PUBLIC imgui imgui/backends/)



# Link to the actual SDL3 library.
target_link_libraries(hello PRIVATE SDL3::SDL3)

# assimp
find_package(assimp REQUIRED)
target_link_libraries(hello PRIVATE assimp::assimp)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(hello PRIVATE glm::glm)


