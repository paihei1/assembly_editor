cmake_minimum_required(VERSION 3.16)
project(hello)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Use vendored libs
set(SDLTTF_VENDORED ON)

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# This assumes the SDL_ttf source is available in vendored/SDL_ttf
add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

include_directories(vendored/imgui/imgui vendored/imgui/imgui/backends vendored)

set(SOURCES hello.cpp 
	instructions.cpp
	draw_main.cpp
	changes.cpp
	main_state.cpp
	vendored/imgui/imgui/imgui.cpp
	vendored/imgui/imgui/imgui_draw.cpp
	vendored/imgui/imgui/imgui_tables.cpp
	vendored/imgui/imgui/imgui_widgets.cpp
	vendored/imgui/imgui/backends/imgui_impl_sdl3.cpp
	vendored/imgui/imgui/backends/imgui_impl_sdlrenderer3.cpp)

# Create your game executable target as usual
add_executable(hello ${SOURCES})

set_property(TARGET hello PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


# Link to the actual SDL3 library.
target_link_libraries(hello PRIVATE SDL3_ttf::SDL3_ttf SDL3::SDL3)