cmake_minimum_required(VERSION 3.20)
project(vlc_plugin_usm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(ENV{PKG_CONFIG_PATH} "M:/msys64/mingw64/lib/pkgconfig")

find_package(PkgConfig REQUIRED)
pkg_check_modules(VLC_PLUGIN REQUIRED vlc-plugin)
pkg_check_modules(VPX REQUIRED vpx)

add_library(usm_core STATIC
    src/usm_chunk.c
    src/usm_crypto.c
    src/usm_ivf.c
    src/usm_keys.c
    src/usm_vp9.c
)
set_target_properties(usm_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(usm_core PUBLIC src)
target_compile_options(usm_core PRIVATE -Wall -Wextra -Wpedantic)

if(WIN32)
    list(APPEND PLUGIN_SOURCES src/dll_entry.c)
endif()

add_library(usm_plugin MODULE
    src/usm_plugin.c
    src/usm_vlc_seek.c
    src/usm_vlc_stream.c
    src/usm_vlc_video.c
    ${PLUGIN_SOURCES}
)
set_target_properties(usm_plugin PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "usm_plugin"
)
target_include_directories(usm_plugin PRIVATE ${VLC_PLUGIN_INCLUDE_DIRS} src)
target_include_directories(usm_plugin PRIVATE ${VPX_INCLUDE_DIRS})
target_compile_options(usm_plugin PRIVATE ${VLC_PLUGIN_CFLAGS_OTHER} -Wall -Wextra)
target_link_libraries(usm_plugin PRIVATE usm_core ${VLC_PLUGIN_LIBRARIES} ${VPX_LIBRARIES})
target_link_directories(usm_plugin PRIVATE ${VLC_PLUGIN_LIBRARY_DIRS} ${VPX_LIBRARY_DIRS})
target_compile_definitions(usm_plugin PRIVATE _GNU_SOURCE MODULE_STRING="usm")

execute_process(
    COMMAND pkg-config --variable=pluginsdir vlc-plugin
    OUTPUT_VARIABLE VLC_PLUGIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT VLC_PLUGIN_DIR)
    set(VLC_PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/vlc/plugins")
endif()
install(TARGETS usm_plugin LIBRARY DESTINATION "${VLC_PLUGIN_DIR}/demux")

include(CTest)
if(BUILD_TESTING)
    add_executable(usm_core_tests
        tests/usm_core_tests.c
    )
    target_link_libraries(usm_core_tests PRIVATE usm_core)
    target_compile_options(usm_core_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME usm_core_tests COMMAND usm_core_tests)
endif()

