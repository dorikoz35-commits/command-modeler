cmake_minimum_required(VERSION 3.10)
project(CMDMODELER C CXX)

set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

add_library(main SHARED
    src/main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_opengl3.cpp)

target_include_directories(main PRIVATE SDL/include imgui imgui/backends src)
target_compile_definitions(main PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main SDL2 GLESv3 log android)
