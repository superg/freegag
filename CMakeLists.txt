cmake_minimum_required(VERSION 3.28)

set(FREEGAG_PROJECT_NAME "freegag" CACHE STRING "Project name")
set(FREEGAG_VERSION "0.8.4" CACHE STRING "Project version")
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
    enable_language(OBJCXX)
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
set(SDL_DIALOG ON CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
  URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.9b.tar.gz
  URL_HASH SHA256=21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(freegag_xtet_resource STATIC "tools/xtet_resource_extractor/main.cpp")
target_compile_definitions(freegag_xtet_resource PRIVATE FREEGAG_EXTRACTOR_LIBRARY=1)
target_include_directories(freegag_xtet_resource PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tools/xtet_resource_extractor")

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

install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
  DESTINATION "."
)
install(FILES "${imgui_SOURCE_DIR}/LICENSE.txt" DESTINATION "licenses" RENAME "Dear-ImGui-LICENSE.txt")
install(FILES "${nlohmann_json_SOURCE_DIR}/LICENSE.MIT" DESTINATION "licenses" RENAME "nlohmann-json-LICENSE.MIT")

if(FREEGAG_TARGET_LINUX)
  foreach(FREEGAG_ICON_SIZE 16 32 48 64 128 256 512 1024)
    install(
      FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/icons/FreeGAG-${FREEGAG_ICON_SIZE}.png"
      DESTINATION "share/icons/hicolor/${FREEGAG_ICON_SIZE}x${FREEGAG_ICON_SIZE}/apps"
      RENAME "io.github.superg.FreeGAG.png"
    )
  endforeach()
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/io.github.superg.FreeGAG.desktop" DESTINATION "share/applications")
endif()

set(CPACK_GENERATOR "ZIP")
include(CPack)
