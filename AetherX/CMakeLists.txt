cmake_minimum_required(VERSION 3.20)
project(NullClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── MinHook (local) ─────────────────────────────────────────────────────
add_subdirectory(minhook-master)

# ── ImGui sources ────────────────────────────────────────────────────────
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

# ── Project sources ──────────────────────────────────────────────────────
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")
# Ensure nofall.cpp is explicitly included if GLOB fails for some reason (it should be fine though)
# But actually the GLOB should pick it up. Let's check why it didn't.

# ── DLL target ───────────────────────────────────────────────────────────
add_library(null SHARED ${PROJECT_SOURCES} ${IMGUI_SOURCES})

target_include_directories(null PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/jni_headers
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(null PRIVATE
    minhook
    opengl32
)

target_compile_definitions(null PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# Disable MSVC warnings for external code
target_compile_options(null PRIVATE /W3 /wd4005 /wd4996)
