set(XTET_ASSET_DIR "" CACHE PATH "Directory containing XTETDLL.SFS produced by xtet_resource_extractor")

if(NOT XTET_ASSET_DIR)
  message(FATAL_ERROR "XTET_ASSET_DIR is required. Run xtet_resource_extractor on the original XTETDLL.DLL from that directory, then configure this path to it.")
endif()

get_filename_component(XTET_ASSET_DIR_ABSOLUTE "${XTET_ASSET_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
set(XTET_RESOURCE_PAYLOAD "${XTET_ASSET_DIR_ABSOLUTE}/XTETDLL.SFS")
set(XTET_DEPLOY_PAYLOAD "${XTET_RESOURCE_PAYLOAD}" PARENT_SCOPE)

if(NOT EXISTS "${XTET_RESOURCE_PAYLOAD}")
  message(FATAL_ERROR "Prepared XTET resource file is missing: ${XTET_RESOURCE_PAYLOAD}")
endif()

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
    resource_provider.cpp
    rli_animation.cpp
    runtime_tables.cpp
    scene_description.cpp
    sfs_archive.cpp
    sfs_archive_test.cpp
  )
  target_compile_features(xtet_sfs_test PRIVATE cxx_std_23)
  target_include_directories(xtet_sfs_test PRIVATE "${CMAKE_SOURCE_DIR}")
  target_link_libraries(xtet_sfs_test PRIVATE ZLIB::ZLIBSTATIC)
  if(MSVC)
    target_compile_options(xtet_sfs_test PRIVATE /W4 /permissive- /EHsc)
  else()
    target_compile_options(xtet_sfs_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME xtet_sfs COMMAND xtet_sfs_test "${XTET_RESOURCE_PAYLOAD}")
  add_custom_command(TARGET xtet_sfs_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${XTET_RESOURCE_PAYLOAD}" "$<TARGET_FILE_DIR:xtet_sfs_test>/XTETDLL.SFS"
  )
endif()
