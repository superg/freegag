cmake_minimum_required(VERSION 3.20)
project(freegag LANGUAGES CXX RC)

include(CTest)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "freegag reconstructs 32-bit Win32 binaries; configure with -A Win32")
endif()

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
  message(FATAL_ERROR "xtet_loader requires Windows")
endif()

set(GAG_RESOURCE_DIR "" CACHE PATH "Optional directory containing a locally extracted gag_resources.rc and its original GAG.EXE assets")

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
target_compile_features(gag PRIVATE cxx_std_17)
target_compile_definitions(gag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(gag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gag PRIVATE user32 gdi32 winmm advapi32 comdlg32)

add_executable(gag_startup_test
  src/cdf_archive.cpp
  src/startup.cpp
  src/startup_test.cpp
)
target_compile_features(gag_startup_test PRIVATE cxx_std_17)
target_compile_definitions(gag_startup_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING)
target_include_directories(gag_startup_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gag_startup_test PRIVATE user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_test COMMAND gag_startup_test)

if(GAG_RESOURCE_DIR)
  add_executable(gag_resource_test src/resource_test.cpp)
  target_compile_features(gag_resource_test PRIVATE cxx_std_17)
  target_compile_definitions(gag_resource_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(gag_resource_test PRIVATE user32)
  add_dependencies(gag_resource_test gag)
  add_test(NAME gag_resource_test COMMAND gag_resource_test "${CMAKE_CURRENT_SOURCE_DIR}/data/orig/GAG.EXE" "$<TARGET_FILE:gag>")
endif()

if(MSVC)
  target_compile_options(gag PRIVATE /W4 /permissive-)
  target_compile_options(gag_startup_test PRIVATE /W4 /permissive-)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE /W4 /permissive-)
  endif()
else()
  target_compile_options(gag PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_test PRIVATE -Wall -Wextra -Wpedantic)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_subdirectory(tools/cdf_extractor)
add_subdirectory(tools/xtet_resource_extractor)

option(XTET_BUILD_RECONSTRUCTED_DLL "Build the reconstructed XTETDLL.DLL" ON)
if(XTET_BUILD_RECONSTRUCTED_DLL)
  add_subdirectory(xtet)
endif()

add_executable(xtet_loader WIN32 xtet/loader/main.cpp)
target_compile_features(xtet_loader PRIVATE cxx_std_17)
target_compile_definitions(xtet_loader PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(xtet_loader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(xtet_loader PRIVATE user32 gdi32 winmm)
if(TARGET xtet)
  add_dependencies(xtet_loader xtet)
endif()

if(MSVC)
  target_compile_options(xtet_loader PRIVATE /W4 /permissive-)
else()
  target_compile_options(xtet_loader PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(XTET_PALETTE "${CMAKE_CURRENT_SOURCE_DIR}/data/Gag01/VE-GBNEW.BMP")
if(TARGET xtet)
  add_custom_command(TARGET xtet_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:xtet>" "$<TARGET_FILE_DIR:xtet_loader>/XTETDLL.DLL")
endif()
if(EXISTS "${XTET_PALETTE}")
  add_custom_command(TARGET xtet_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${XTET_PALETTE}" "$<TARGET_FILE_DIR:xtet_loader>/VE-GBNEW.BMP")
endif()
