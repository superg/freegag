set(XTET_ASSET_DIR "" CACHE PATH "Directory prepared by xtet_resource_extractor")

if(NOT XTET_ASSET_DIR)
  message(FATAL_ERROR "XTET_ASSET_DIR is required. Build xtet_resource_extractor, run it on the original XTETDLL.DLL, then configure this directory to its output.")
endif()

get_filename_component(XTET_ASSET_DIR_ABSOLUTE "${XTET_ASSET_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
set(XTET_RESOURCE_SCRIPT "${XTET_ASSET_DIR_ABSOLUTE}/XTETSFS.rc")
set(XTET_RESOURCE_PAYLOAD "${XTET_ASSET_DIR_ABSOLUTE}/XTETSFS.bin")
set(XTET_RESOURCE_METADATA "${XTET_ASSET_DIR_ABSOLUTE}/XTETSFS.json")

foreach(required_file IN ITEMS "${XTET_RESOURCE_SCRIPT}" "${XTET_RESOURCE_PAYLOAD}" "${XTET_RESOURCE_METADATA}")
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "Prepared XTET resource file is missing: ${required_file}")
  endif()
endforeach()

set_source_files_properties("${XTET_RESOURCE_SCRIPT}" PROPERTIES OBJECT_DEPENDS "${XTET_RESOURCE_PAYLOAD}")

add_library(xtet SHARED
  action_definitions.cpp
  action_definitions.h
  audio_coordinator.cpp
  audio_coordinator.h
  asset_decoders.cpp
  asset_decoders.h
  asset_manifest.cpp
  asset_manifest.h
  api.h
  dll.cpp
  figurine_geometry.cpp
  figurine_geometry.h
  game_runtime.cpp
  game_runtime.h
  game_timing.cpp
  game_timing.h
  game_worker.cpp
  game_worker.h
  game_progress.cpp
  game_progress.h
  gameplay_state.cpp
  gameplay_state.h
  indexed_renderer.cpp
  indexed_renderer.h
  match_animation.cpp
  match_animation.h
  resource_provider.cpp
  resource_provider.h
  rli_animation.cpp
  rli_animation.h
  runtime_tables.cpp
  runtime_tables.h
  scene_description.cpp
  scene_description.h
  sfs_archive.cpp
  sfs_archive.h
  xtet.def
  "${XTET_RESOURCE_SCRIPT}"
  "${CMAKE_SOURCE_DIR}/tools/cdf_extractor/inflate.c"
)

set_source_files_properties("${CMAKE_SOURCE_DIR}/tools/cdf_extractor/inflate.c" PROPERTIES LANGUAGE CXX)

target_compile_features(xtet PRIVATE cxx_std_17)
target_compile_definitions(xtet PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(xtet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(xtet PRIVATE winmm)
set_target_properties(xtet PROPERTIES PREFIX "" OUTPUT_NAME "XTETDLL")

if(MSVC)
  target_compile_options(xtet PRIVATE /W4 /permissive-)
  target_link_options(xtet PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/xtet.def")
else()
  target_compile_options(xtet PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
  add_executable(xtet_sfs_test
    action_definitions.cpp
    audio_coordinator.cpp
    asset_decoders.cpp
    asset_manifest.cpp
    figurine_geometry.cpp
    game_runtime.cpp
    game_timing.cpp
    game_worker.cpp
    game_progress.cpp
    gameplay_state.cpp
    indexed_renderer.cpp
    match_animation.cpp
    rli_animation.cpp
    runtime_tables.cpp
    scene_description.cpp
    sfs_archive.cpp
    sfs_archive_test.cpp
    "${CMAKE_SOURCE_DIR}/tools/cdf_extractor/inflate.c"
  )
  set_source_files_properties("${CMAKE_SOURCE_DIR}/tools/cdf_extractor/inflate.c" PROPERTIES LANGUAGE CXX)
  target_compile_features(xtet_sfs_test PRIVATE cxx_std_17)
  target_include_directories(xtet_sfs_test PRIVATE "${CMAKE_SOURCE_DIR}")
  if(MSVC)
    target_compile_options(xtet_sfs_test PRIVATE /W4 /permissive-)
  else()
    target_compile_options(xtet_sfs_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME xtet_sfs COMMAND xtet_sfs_test "${XTET_RESOURCE_PAYLOAD}")
endif()
