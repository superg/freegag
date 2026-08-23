cmake_minimum_required(VERSION 4.4)
project(freegag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
