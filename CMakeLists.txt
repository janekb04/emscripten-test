cmake_minimum_required(VERSION 3.21)
project(emscripten-test)

add_executable(main src/main.cpp src/ImGuiFileDialog.cpp)
target_compile_features(main PRIVATE cxx_std_17)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfwpp)
add_subdirectory(external/imgui)
target_link_libraries(main PRIVATE GLFWPP Imgui)

file(GLOB_RECURSE RESOURCE_FILES ${PROJECT_SOURCE_DIR}/res/*)
add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)
add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${RESOURCE_FILES}
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_definitions(main PRIVATE "__EMSCRIPTEN__")

    set(CMAKE_CXX_LINKER_WRAPPER_FLAG "")
    target_link_options(main PRIVATE "LINKER:SHELL:-sEXPORTED_FUNCTIONS=[_malloc,_main] --preload-file ${PROJECT_SOURCE_DIR}/res@res")
    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/index.html
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html)
else()
    add_subdirectory(external/glew-cmake)
    target_link_libraries(main PRIVATE libglew_static)
    target_compile_definitions(main PRIVATE GLEW_STATIC)
endif()