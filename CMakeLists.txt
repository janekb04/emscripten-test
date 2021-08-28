cmake_minimum_required(VERSION 3.21)
project(emscripten-test)

add_executable(main main.cpp)
target_compile_features(main PRIVATE cxx_std_17)

add_subdirectory(external/glfwpp)
target_link_libraries(main PRIVATE GLFWPP)