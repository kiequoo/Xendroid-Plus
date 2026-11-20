# Injected via CMAKE_PROJECT_INCLUDE when building the xenia tree standalone
# (i.e. not via the outer Android CMakeLists that normally sets up zerofg).
# Provides the zerofg::zerofg target that xenia-ui-vulkan links against.
#
# CMAKE_CURRENT_SOURCE_DIR here is the xenia tree root, so ../zerofg is
# emulator-core/src/main/cpp/zerofg/.
if(NOT TARGET zerofg)
    set(ZEROFG_VULKAN_HEADERS_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include"
        CACHE STRING "Vulkan headers for ZeroFG" FORCE)
    set(ZEROFG_PREBUILT_SHADERS ON CACHE BOOL
        "Use ZeroFG prebuilt SPIR-V shaders" FORCE)
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/../zerofg"
        "${CMAKE_CURRENT_BINARY_DIR}/zerofg"
        EXCLUDE_FROM_ALL
    )
endif()
