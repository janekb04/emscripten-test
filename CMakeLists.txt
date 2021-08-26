cmake_minimum_required(VERSION 3.21)

#set(TARGET_PLATFORM_EMSCRIPTEN ON)

if (TARGET_PLATFORM_EMSCRIPTEN)
    set(EMSDK "C:/Users/Jan/emsdk")
    set(CMAKE_TOOLCHAIN_FILE "${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    set(CMAKE_CROSSCOMPILING_EMULATOR "${EMSDK}/node/14.15.5_64bit/bin/node.exe")
endif()

project(emscripten-test)

add_executable(main main.cpp)
target_compile_features(main PRIVATE cxx_std_17)
if (TARGET_PLATFORM_EMSCRIPTEN)
    target_include_directories(main PRIVATE "${EMSDK}/upstream/emscripten/system/include")
    set_target_properties(main PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s LLD_REPORT_UNDEFINED")
endif()