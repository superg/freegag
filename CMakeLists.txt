cmake_minimum_required(VERSION 3.20)
project(freegag LANGUAGES C CXX RC)

include(CTest)
include(FetchContent)

# Code formatting targets, following the neighboring redumper project.
find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
  file(GLOB_RECURSE ALL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/xtet/*.cpp"
    "${PROJECT_SOURCE_DIR}/xtet/*.h"
    "${PROJECT_SOURCE_DIR}/xtet/*.hpp"
    "${PROJECT_SOURCE_DIR}/tools/*.cc"
    "${PROJECT_SOURCE_DIR}/tools/*.cpp"
    "${PROJECT_SOURCE_DIR}/tools/*.h"
    "${PROJECT_SOURCE_DIR}/tools/*.hh"
    "${PROJECT_SOURCE_DIR}/tools/*.hpp"
  )

  add_custom_target(format COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES})
  add_custom_target(check-format COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES})
else()
  message(WARNING "clang-format not found, source code formatting targets will not be available")
endif()

if(NOT WIN32)
  message(FATAL_ERROR "freegag requires Windows")
endif()

set(GAG_RESOURCE_DIR "" CACHE PATH "Optional directory containing a locally extracted gag_resources.rc and its original GAG.EXE assets")
set(GAG_LOCAL_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/gag-resources")
if(NOT GAG_RESOURCE_DIR AND EXISTS "${GAG_LOCAL_RESOURCE_DIR}/gag_resources.rc")
  set(GAG_RESOURCE_DIR "${GAG_LOCAL_RESOURCE_DIR}" CACHE PATH "Optional directory containing a locally extracted gag_resources.rc and its original GAG.EXE assets" FORCE)
endif()
option(FREEGAG_WINDOWS_FIXES "Enable compatibility fixes for modern Windows" ON)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlib
  URL https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
  URL_HASH SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
)
FetchContent_MakeAvailable(zlib)

add_subdirectory(xtet)

set(GAG_SOURCES
  src/cdf_archive.cpp
  src/main.cpp
  src/startup.cpp
)
if(GAG_RESOURCE_DIR)
  if(NOT EXISTS "${GAG_RESOURCE_DIR}/gag_resources.rc")
    message(FATAL_ERROR "GAG_RESOURCE_DIR must contain gag_resources.rc")
  endif()
  list(APPEND GAG_SOURCES "${GAG_RESOURCE_DIR}/gag_resources.rc")
endif()

add_executable(gag WIN32 ${GAG_SOURCES})
target_compile_features(gag PRIVATE cxx_std_23)
target_compile_definitions(gag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
if(FREEGAG_WINDOWS_FIXES)
  target_compile_definitions(gag PRIVATE FREEGAG_WINDOWS_FIXES=1)
endif()
target_include_directories(gag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gag PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_custom_command(TARGET gag POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${XTET_DEPLOY_PAYLOAD}" "$<TARGET_FILE_DIR:gag>/XTETDLL.SFS"
)

add_executable(gag_startup_test
  src/cdf_archive.cpp
  src/startup.cpp
  src/startup_test.cpp
)
target_compile_features(gag_startup_test PRIVATE cxx_std_23)
target_compile_definitions(gag_startup_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING FREEGAG_ORIGINAL_GAG01_CDF="${CMAKE_CURRENT_SOURCE_DIR}/data/orig/Gag01.cdf")
target_include_directories(gag_startup_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gag_startup_test PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_test COMMAND gag_startup_test)

add_executable(gag_startup_windows_fixes_test
  src/cdf_archive.cpp
  src/startup.cpp
  src/startup_test.cpp
)
target_compile_features(gag_startup_windows_fixes_test PRIVATE cxx_std_23)
target_compile_definitions(gag_startup_windows_fixes_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING FREEGAG_WINDOWS_FIXES=1 FREEGAG_ORIGINAL_GAG01_CDF="${CMAKE_CURRENT_SOURCE_DIR}/data/orig/Gag01.cdf")
target_include_directories(gag_startup_windows_fixes_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gag_startup_windows_fixes_test PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_windows_fixes_test COMMAND gag_startup_windows_fixes_test)

if(GAG_RESOURCE_DIR)
  add_executable(gag_resource_test src/resource_test.cpp)
  target_compile_features(gag_resource_test PRIVATE cxx_std_23)
  target_compile_definitions(gag_resource_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(gag_resource_test PRIVATE user32)
  add_dependencies(gag_resource_test gag)
  add_test(NAME gag_resource_test COMMAND gag_resource_test "${CMAKE_CURRENT_SOURCE_DIR}/data/orig/GAG.EXE" "$<TARGET_FILE:gag>")
endif()

if(MSVC)
  target_compile_options(gag PRIVATE /W4 /permissive- /EHsc)
  target_compile_options(gag_startup_test PRIVATE /W4 /permissive- /EHsc)
  target_compile_options(gag_startup_windows_fixes_test PRIVATE /W4 /permissive- /EHsc)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE /W4 /permissive- /EHsc)
  endif()
else()
  target_compile_options(gag PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_test PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_windows_fixes_test PRIVATE -Wall -Wextra -Wpedantic)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_subdirectory(tools/xtet_resource_extractor)
