cmake_minimum_required(VERSION 3.20)
project(vlc_plugin_usm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 从 MSYS2 MinGW64 shell 构建时 pkg-config 会自动找到 vlc-plugin/vpx。
# 若在其它环境构建，通过环境变量 PKG_CONFIG_PATH 或 -DCMAKE_PREFIX_PATH 指定，
# 不要在此处硬编码本机路径。
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
    # VLC 的 vlc_threads.h 在 Windows 分支里有个 static inline vlc_poll()，它调用 POSIX
    # poll()，而 mingw 只有 WSAPoll。该 inline 本插件不会调用（不会产生链接引用），但
    # GCC 14+ 默认把“隐式声明 poll”当硬错误，导致仅含 VLC 头就编不过。降级为警告即可。
    set(PLUGIN_WIN_C_FLAGS -Wno-error=implicit-function-declaration)
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
target_compile_options(usm_plugin PRIVATE ${VLC_PLUGIN_CFLAGS_OTHER} -Wall -Wextra ${PLUGIN_WIN_C_FLAGS})
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

