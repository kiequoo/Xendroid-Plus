# Xenia Nexus — standalone SDL3 launcher for the bundled xenia_edge emulator.
#
# Sources vendored and adapted from xbased (crazyriddler/xbased, GPL-3.0).
# SDL3 reused from the xenia tree (third_party/SDL3, 3.4.8 — API-compatible
# with xbased's 3.4.18).
# stb (public domain / MIT) vendored under third_party/stb/.
#
# Build from the repo root:
#   cmake -S linux/nexus -B linux/nexus/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build linux/nexus/build --config Release
cmake_minimum_required(VERSION 3.20)
project(xenia_nexus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── SDL3 ─────────────────────────────────────────────────────────────────────
# Reuse the SDL3 source tree bundled with xenia (3.4.8, same minor as xbased's
# 3.4.18 — no API differences within a minor version).
get_filename_component(XENIA_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../emulator-core/src/main/cpp/xenia"
    ABSOLUTE)

set(SDL_SHARED          OFF CACHE BOOL "" FORCE)
set(SDL_STATIC          ON  CACHE BOOL "" FORCE)
set(SDL_STATIC_PIC      ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY    OFF CACHE BOOL "" FORCE)
set(SDL_TESTS           OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL         OFF CACHE BOOL "" FORCE)
# Suppress SDL's own install rules so they don't interfere with ours.
set(SDL3_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${XENIA_ROOT}/third_party/SDL3" sdl3-nexus EXCLUDE_FROM_ALL)

# ── stb ──────────────────────────────────────────────────────────────────────
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")

# ── Sources ──────────────────────────────────────────────────────────────────
add_executable(xenia-nexus
    nexus_main.c
    xbox_data_shim.c
    ui/ui.c
    ui/dash.c
    ui/library.c
    ui/dash_browse.c
    ui/dash_cover.c
    ui/dash_find.c
    ui/dash_settings.c   # stub (Phase 1)
    ui/input_map.c
    ui/pointer.c
    ui/sound.c
    ui/config.c
    ui/i18n.c
)

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(xenia-nexus PRIVATE
    # nexus root → nexus_main.c can #include "ui/dash.h" etc.
    "${CMAKE_CURRENT_SOURCE_DIR}"
    # ui/ dir → vendored files include each other without path prefix
    "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    # stb headers (stb_truetype.h, stb_image.h, stb_image_write.h)
    "${STB_DIR}"
)

# ── Link ──────────────────────────────────────────────────────────────────────
target_link_libraries(xenia-nexus PRIVATE
    SDL3::SDL3-static
    m
    pthread
    dl
)

# ── Platform flags ────────────────────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(xenia-nexus PRIVATE _GNU_SOURCE)
    target_compile_options(xenia-nexus PRIVATE
        -g1
        -fPIE
        -Wall
        -Wno-unused-function
        # stb generates many warnings — suppress for the vendored headers
        -Wno-sign-compare
        -Wno-unused-but-set-variable
    )
    target_link_options(xenia-nexus PRIVATE
        -pie
        -static-libgcc
    )
endif()
