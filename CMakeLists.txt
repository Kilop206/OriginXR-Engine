cmake_minimum_required(VERSION 3.20)
project(OriginXR VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ORIGINXR_BUILD_GRAPHICS "Build the OpenGL sandbox application" ON)
option(ORIGINXR_BUILD_TESTS "Build headless engine core tests" ON)
option(ORIGINXR_BUILD_EDITOR "Build Dear ImGui scene editor (requires graphics)" ON)

add_library(originxr_core STATIC
    src/Math.cpp
    src/Camera.cpp
    src/Scene.cpp
    src/SceneIO.cpp
    src/Assets.cpp
    src/XRMath.cpp
    src/Input.cpp
    src/editor/EditorModel.cpp
)
add_library(OriginXR::Core ALIAS originxr_core)
target_include_directories(originxr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(originxr_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(originxr_core PRIVATE /W4)
else()
    target_compile_options(originxr_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ORIGINXR_BUILD_EDITOR AND NOT ORIGINXR_BUILD_GRAPHICS)
    message(STATUS "Editor disabled because ORIGINXR_BUILD_GRAPHICS=OFF")
endif()

if(ORIGINXR_BUILD_GRAPHICS)
    # Prefer system GLFW; otherwise fetch a pinned GLFW release.
    find_package(glfw3 3.3 QUIET)
    if(TARGET glfw)
        set(ORIGINXR_GLFW_TARGET glfw)
    elseif(TARGET glfw3::glfw)
        set(ORIGINXR_GLFW_TARGET glfw3::glfw)
    else()
        include(FetchContent)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glfw)
        set(ORIGINXR_GLFW_TARGET glfw)
    endif()

    find_package(OpenGL REQUIRED)
    add_library(originxr_graphics STATIC src/Renderer.cpp src/Engine.cpp)
    add_library(OriginXR::Graphics ALIAS originxr_graphics)
    target_include_directories(originxr_graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(originxr_graphics PUBLIC originxr_core PRIVATE ${ORIGINXR_GLFW_TARGET} OpenGL::GL)

    add_executable(originxr_sandbox apps/sandbox.cpp)
    target_link_libraries(originxr_sandbox PRIVATE originxr_graphics)
    if(WIN32)
        set_target_properties(originxr_sandbox PROPERTIES WIN32_EXECUTABLE FALSE)
    endif()
    # UI is kept in its own executable; applications can use the core/renderer without ImGui.
    if(ORIGINXR_BUILD_EDITOR)
        include(FetchContent)
        FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.9b
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(imgui)
        add_library(originxr_imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(originxr_imgui PUBLIC
            ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
        target_link_libraries(originxr_imgui PUBLIC ${ORIGINXR_GLFW_TARGET} OpenGL::GL)
        add_executable(originxr_editor apps/editor.cpp)
        target_link_libraries(originxr_editor PRIVATE originxr_graphics originxr_imgui ${ORIGINXR_GLFW_TARGET} OpenGL::GL)
        if(WIN32)
            set_target_properties(originxr_editor PROPERTIES WIN32_EXECUTABLE FALSE)
        endif()
        add_custom_command(TARGET originxr_editor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:originxr_editor>/scenes
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes $<TARGET_FILE_DIR:originxr_editor>/scenes
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/meshes $<TARGET_FILE_DIR:originxr_editor>/meshes
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures $<TARGET_FILE_DIR:originxr_editor>/textures
        )
    endif()

    if(MSVC)
        target_compile_options(originxr_graphics PRIVATE /W4)
    else()
        target_compile_options(originxr_graphics PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    # Copy demo assets alongside sandbox; scene files reference meshes/ and textures/.
    add_custom_command(TARGET originxr_sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:originxr_sandbox>/scenes
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:originxr_sandbox>/meshes
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:originxr_sandbox>/textures
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/demo.oxscene
            $<TARGET_FILE_DIR:originxr_sandbox>/scenes/demo.oxscene
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/meshes $<TARGET_FILE_DIR:originxr_sandbox>/meshes
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures $<TARGET_FILE_DIR:originxr_sandbox>/textures
    )
endif()

if(ORIGINXR_BUILD_TESTS)
    enable_testing()
    add_executable(originxr_tests tests/test_core.cpp)
    target_link_libraries(originxr_tests PRIVATE originxr_core)
    target_compile_definitions(originxr_tests PRIVATE ORIGINXR_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME originxr_core_tests COMMAND originxr_tests)
    add_executable(originxr_assets_tests tests/test_assets.cpp)
    target_link_libraries(originxr_assets_tests PRIVATE originxr_core)
    target_compile_definitions(originxr_assets_tests PRIVATE ORIGINXR_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME originxr_assets_tests COMMAND originxr_assets_tests)
    add_executable(originxr_editor_tests tests/test_editor.cpp)
    target_link_libraries(originxr_editor_tests PRIVATE originxr_core)
    target_compile_definitions(originxr_editor_tests PRIVATE ORIGINXR_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME originxr_editor_tests COMMAND originxr_editor_tests)
endif()
