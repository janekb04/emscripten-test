cmake_minimum_required(VERSION 3.21)

project(emscripten-test)

add_executable(main main.cpp)
target_compile_features(main PRIVATE cxx_std_17)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_include_directories(main PRIVATE "${EMSDK}/upstream/emscripten/system/include")
    set_target_properties(main PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s LLD_REPORT_UNDEFINED")
endif()