cmake_minimum_required(VERSION 3.21)
project(emscripten-test)

add_executable(main main.cpp)
target_compile_features(main PRIVATE cxx_std_17)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfwpp)
add_subdirectory(external/imgui)
target_link_libraries(main PRIVATE GLFWPP Imgui)
set(CMAKE_CXX_LINKER_WRAPPER_FLAG "")
target_link_options(GLFWPP INTERFACE "LINKER:SHELL:-sEXPORTED_FUNCTIONS=[_malloc,_main]")