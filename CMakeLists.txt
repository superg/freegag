cmake_minimum_required(VERSION 3.20)
project(freegag LANGUAGES CXX RC)

include(CTest)

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

add_subdirectory(tools/cdf_extractor)
add_subdirectory(tools/xtet_resource_extractor)

option(XTET_BUILD_RECONSTRUCTED_DLL "Build the reconstructed XTETDLL.DLL" ON)
if(XTET_BUILD_RECONSTRUCTED_DLL)
  add_subdirectory(xtet)
endif()

add_executable(xtet_loader WIN32 src/main.cpp)
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
