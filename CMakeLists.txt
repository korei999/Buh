cmake_minimum_required(VERSION 3.25)

file(STRINGS name NAME)
message(STATUS "NAME: '${NAME}'")

project(${NAME} VERSION 0.1 LANGUAGES CXX C)
message(STATUS "CMAKE_PROJECT_VERSION: '${CMAKE_PROJECT_VERSION}'")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "CMAKE_C_COMPILER_ID: '${CMAKE_C_COMPILER_ID}'")
message(STATUS "CMAKE_CXX_COMPILER_ID: '${CMAKE_CXX_COMPILER_ID}'")

set(CMAKE_CXX_STANDARD 20)

message(STATUS "CMAKE_CXX_STANDARD: '${CMAKE_CXX_STANDARD}'")

set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# add_compile_options(-ftime-trace)

option(OPT_SSE4_2 "" OFF)
option(OPT_AVX2 "" OFF)
option(OPT_MARCH_NATIVE "" OFF)

option(OPT_MIMALLOC "" OFF)
option(OPT_SW "" OFF)
option(OPT_GL "" ON)
option(OPT_FAST_MATH "" OFF)
option(OPT_PRECOMPILE_ADT "" OFF)
option(OPT_ANALYZE "" OFF)

if (OPT_ANALYZE)
    add_compile_options(-fanalyzer)
endif()

if (OPT_MIMALLOC)
    include(FetchContent)
    FetchContent_Declare(
        MIMALLOC
        GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
        GIT_TAG dev3
    )
    FetchContent_MakeAvailable(MIMALLOC)
    FetchContent_GetProperties(MIMALLOC
        SOURCE_DIR MIMALLOC_SOURCE_DIR
        BINARY_DIR MIMALLOC_BINARY_DIR
    )

    add_definitions("-DADT_USE_MIMALLOC")
endif()

if (OPT_MARCH_NATIVE)
    if(MSVC)
    else()
        add_compile_options(-march=native)
    endif()
endif()

if (OPT_SSE4_2)
    add_definitions("-DADT_SSE4_2")

    if (MSVC)
        add_compile_options(/arch:SSE4.2)
    else()
        add_compile_options(-msse4.2)
    endif()
endif()

if (OPT_AVX2)
    add_definitions("-DADT_AVX2")

    if (MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-mavx2 -mfma)
    endif()
endif()

if (CMAKE_BUILD_TYPE MATCHES "DebugMingw" OR CMAKE_BUILD_TYPE MATCHES "ReleaseMingw")
    set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

    set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

    set(CMAKE_SYSTEM_NAME "Windows")

    if (CMAKE_BUILD_TYPE MATCHES "DebugMingw")
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    add_definitions("-DADT_STD_TYPES")
endif()

include_directories(BEFORE "libs")
include_directories(BEFORE "src")
include_directories(BEFORE "assets")

add_definitions("-DADT_LOGS_LESS_TYPING")
add_definitions("-DADT_DEFER_LESS_TYPING")
add_definitions("-DADT_JSON_USE_PRINTF")

if (CMAKE_BUILD_TYPE MATCHES "DebugO1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # doesn't allow to override
        add_compile_options(/Oi)
    else()
        add_compile_options(-O1 -g3)
    endif()

    set(CMAKE_BUILD_TYPE "Debug")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Asan")
    set(CMAKE_BUILD_TYPE "Debug")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(-fsanitize=address)
    else()
        add_compile_options(-O0 -g3 -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)

        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            add_compile_options(-shared-libsan)
            # doesn't work...
            # target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE clang_rt.asan_cxx-x86_64.lib clang_rt.asan-x86_64.lib)
        else()
            add_link_options(-fsanitize=undefined -fsanitize=address)
        endif()
    endif()
endif()

if (CMAKE_BUILD_TYPE MATCHES "Tsan")
    set(CMAKE_BUILD_TYPE "Debug")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(-fsanitize=address)
    else()
        add_compile_options(-O0 -g3 -fno-omit-frame-pointer -fsanitize=thread)

        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            add_compile_options(-shared-libsan)
            # doesn't work...
            # target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE clang_rt.asan_cxx-x86_64.lib clang_rt.asan-x86_64.lib)
        else()
            add_link_options(-fsanitize=thread)
        endif()
    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/EHsc /utf-8)
    add_compile_options(/wd4146 /wd4200 /wd4312)

    # add_compile_options(/openmp)
else()
    add_compile_options(-Wall -Wextra -Wshadow)
    add_compile_options(-Wno-missing-braces)
    add_compile_options(-fms-extensions -fvisibility=hidden)

    # add_compile_options(-fopenmp)
    # add_link_options(-fopenmp)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions("-DADT_LOGS")
    add_definitions("-DADT_DBG_MEMORY")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/wd4267 /wd4101 /wd4200 /wd4244)
    else()
        add_compile_options(
            -Wmissing-field-initializers
            -Wno-unused-parameter
            -Wno-unused-variable
            -Wno-unused-function
            -Wno-unused-but-set-variable
        )
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess")
endif()

message(STATUS "CMAKE_SYSTEM_NAME: '${CMAKE_SYSTEM_NAME}'")
message(STATUS "CMAKE_SOURCE_DIR: '${CMAKE_SOURCE_DIR}'")

add_executable(
    ${CMAKE_PROJECT_NAME}

    src/main.cc
    src/app.cc
    src/frame.cc
    src/Image.cc

    src/ttf/Parser.cc
    src/ttf/Rasterizer.cc

    src/wayland/Client.cc
    src/wayland/ClientInput.cc
    src/wayland/ClientBarOutput.cc
)

if (OPT_PRECOMPILE_ADT)
    file(GLOB_RECURSE ADT_PRECOMPILED_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libs/adt/*.hh")

    if (NOT OPT_MIMALLOC)
        list(FILTER ADT_PRECOMPILED_HEADERS EXCLUDE REGEX ".*/adt/MiMalloc.hh")
    endif()

    target_precompile_headers(
        ${CMAKE_PROJECT_NAME} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${ADT_PRECOMPILED_HEADERS}>"
    )
endif()

if (OPT_MIMALLOC)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE mimalloc-static)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

    include_directories(BEFORE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${WAYLAND_CLIENT_LIBRARIES} 
    )

    set(WLP "./src/wayland")
    set(WLPD "${WLP}/WaylandGlueGenerated")
    execute_process(
        COMMAND pkg-config wayland-protocols --variable=pkgdatadir
        OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --variable=wayland_scanner wayland-scanner
        OUTPUT_VARIABLE WAYLAND_SCANNER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(XDG_SHELL "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    set(WLR_LAYER_SHELL "protocols/wlr-layer-shell-unstable-v1.xml")
    set(DWL_IPC "protocols/dwl-ipc-unstable-kblayout-v2.xml")

    message(STATUS "WLP: '${WLP}'")
    message(STATUS "WLPD: '${WLPD}'")
    message(STATUS "WAYLAND_PROTOCOLS_DIR: '${WAYLAND_PROTOCOLS_DIR}'")
    message(STATUS "WAYLAND_SCANNER: '${WAYLAND_SCANNER}'")
    message(STATUS "XDG_SHELL: '${XDG_SHELL}'")
    message(STATUS "WLR_LAYER_SHELL: '${WLR_LAYER_SHELL}'")
    message(STATUS "DWL_IPC: '${DWL_IPC}'")

    execute_process(COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/${WLPD})
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL} ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${XDG_SHELL} ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${WLR_LAYER_SHELL} ${CMAKE_SOURCE_DIR}/${WLPD}/wlr-layer-shell.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${WLR_LAYER_SHELL} ${CMAKE_SOURCE_DIR}/${WLPD}/wlr-layer-shell.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${DWL_IPC} ${CMAKE_SOURCE_DIR}/${WLPD}/dwl-ipc.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${DWL_IPC} ${CMAKE_SOURCE_DIR}/${WLPD}/dwl-ipc.c)

    target_sources(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${WLPD}/wlr-layer-shell.c
        ${WLPD}/dwl-ipc.c
        ${WLPD}/xdg-shell.c
    )
endif()

# install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
