cmake_minimum_required(VERSION 3.20)
project(freegag LANGUAGES CXX RC)

include(CTest)
include(FetchContent)

set(GAG_RESOURCE_DIR "" CACHE PATH "Optional directory containing a locally extracted gag_resources.rc and its original GAG.EXE assets")
set(GAG_LOCAL_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/gag-resources")
if(NOT GAG_RESOURCE_DIR AND EXISTS "${GAG_LOCAL_RESOURCE_DIR}/gag_resources.rc")
  set(GAG_RESOURCE_DIR "${GAG_LOCAL_RESOURCE_DIR}" CACHE PATH "Optional directory containing a locally extracted gag_resources.rc and its original GAG.EXE assets" FORCE)
endif()
option(FREEGAG_WINDOWS_FIXES "Enable compatibility fixes for modern Windows" ON)

set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlib
  URL https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
  URL_HASH SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
)
FetchContent_MakeAvailable(zlib)

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_target(format COMMAND ${CLANG_FORMAT} -i ${FREEGAG_FORMAT_FILES})
  add_custom_target(check-format COMMAND ${CLANG_FORMAT} --dry-run --Werror ${FREEGAG_FORMAT_FILES})
else()
  message(WARNING "clang-format not found, source code formatting targets will not be available")
endif()

add_subdirectory(src)
add_subdirectory(tools)
