cmake_minimum_required(VERSION 3.28)

set(FREEGAG_PROJECT_NAME "freegag" CACHE STRING "Project name")
set(FREEGAG_VERSION "0.8.3" CACHE STRING "Project version")
project(${FREEGAG_PROJECT_NAME} VERSION ${FREEGAG_VERSION} LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE INTERNAL "Active configuration" FORCE)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FREEGAG_TARGET_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(FREEGAG_TARGET_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(FREEGAG_TARGET_MACOS 1)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(FREEGAG_CLANG_USE_LIBCPP "Link to libc++ instead of system default" OFF)

    if(FREEGAG_CLANG_USE_LIBCPP)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
        add_link_options(-stdlib=libc++)
    endif()

    add_link_options(${FREEGAG_CLANG_LINK_OPTIONS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Werror)
elseif(MSVC)
    # build MT configuration by default
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # multithreaded build
    add_compile_options(/MP)

#    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    # flatten source lists
    file(GLOB_RECURSE files LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
    foreach(i IN LISTS files)
        get_filename_component(source_path "${i}" PATH)
        string(REPLACE "/" "\\" source_path "${source_path}")
        source_group("${source_path}" FILES ${i})
    endforeach()
endif()

include(FetchContent)

set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlib
  URL https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
  URL_HASH SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
)
FetchContent_MakeAvailable(zlib)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
  URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
)
FetchContent_MakeAvailable(SDL3)

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  file(GLOB_RECURSE ALL_SOURCE_FILES
    "src/*.cpp"
    "src/*.h"
    "tools/*.cpp"
    "tools/*.h"
  )

  add_custom_target(format COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES})
  add_custom_target(check-format COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES})
else()
  message(WARNING "clang-format not found, source code formatting targets will not be available")
endif()

add_subdirectory("src")
add_subdirectory("tools")

set(CPACK_GENERATOR "ZIP")
include(CPack)
