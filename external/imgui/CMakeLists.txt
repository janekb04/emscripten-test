add_library (Imgui
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/misc/freetype/imgui_freetype.cpp
        )
target_include_directories(Imgui PUBLIC
        imgui
        imgui/backends
        imgui/misc/freetype
        )
target_link_libraries(Imgui PUBLIC freetype glfw)
target_compile_definitions(Imgui PUBLIC IMGUI_ENABLE_FREETYPE)

if (WIN32 AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
        target_link_libraries(Imgui PUBLIC "Imm32.lib")
endif()