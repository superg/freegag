add_library(xtet_core STATIC
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
)

target_compile_features(xtet_core PRIVATE cxx_std_23)
target_compile_definitions(xtet_core PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(xtet_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(xtet_core PUBLIC ZLIB::ZLIBSTATIC winmm)

if(MSVC)
  target_compile_options(xtet_core PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(xtet_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

list(APPEND FREEGAG_FORMAT_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/action_definitions.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/action_definitions.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/api.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/asset_decoders.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/asset_decoders.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/asset_manifest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/asset_manifest.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/audio_coordinator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/audio_coordinator.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/dll.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/figurine_geometry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/figurine_geometry.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_progress.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_progress.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_runtime.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_runtime.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_timing.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_timing.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_worker.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/game_worker.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/gameplay_state.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/gameplay_state.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/indexed_renderer.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/indexed_renderer.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/match_animation.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/match_animation.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/resource_provider.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/resource_provider.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/rli_animation.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/rli_animation.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/runtime_tables.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/runtime_tables.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/scene_description.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/scene_description.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/sfs_archive.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/sfs_archive.h"
)
set(FREEGAG_FORMAT_FILES "${FREEGAG_FORMAT_FILES}" PARENT_SCOPE)
