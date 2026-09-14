include(CheckIncludeFile)

check_include_file("unistd.h" HAVE_UNISTD_H)
if(MSVC)
  set(NO_OFF_T 1)
endif()
configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/legacy_gzip_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/legacy_gzip/config.h"
)

set(LEGACY_GZIP_CODEC_SOURCES
  "${legacy_gzip_SOURCE_DIR}/bits.c"
  "${legacy_gzip_SOURCE_DIR}/deflate.c"
  "${legacy_gzip_SOURCE_DIR}/inflate.c"
  "${legacy_gzip_SOURCE_DIR}/trees.c"
)

add_library(freegag_legacy_gzip STATIC
  "cdf_format.cpp"
  "cdf_format.h"
  "legacy_gzip_codec.cpp"
  "legacy_gzip_codec.h"
  "legacy_gzip_shim.c"
  "legacy_gzip_shim.h"
  ${LEGACY_GZIP_CODEC_SOURCES}
)
target_include_directories(freegag_legacy_gzip
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/legacy_gzip"
    "${legacy_gzip_SOURCE_DIR}"
)
target_compile_features(freegag_legacy_gzip PUBLIC cxx_std_23)
set_target_properties(freegag_legacy_gzip PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON)
if(MSVC)
  target_compile_definitions(freegag_legacy_gzip PRIVATE _CRT_SECURE_NO_WARNINGS)
  set_source_files_properties(${LEGACY_GZIP_CODEC_SOURCES} PROPERTIES COMPILE_OPTIONS "/w")
else()
  set_source_files_properties(${LEGACY_GZIP_CODEC_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
endif()

add_executable(freegag
  "application.cpp"
  "application.h"
  "application_paths.cpp"
  "application_paths.h"
  "application_types.h"
  "audio.cpp"
  "audio.h"
  "audio_types.h"
  "cdf_archive.cpp"
  "display_host.cpp"
  "display_host.h"
  "display_scene.cpp"
  "display_scene.h"
  "display_types.h"
  "extra/save_load.cpp"
  "extra/save_load.h"
  "extra/gagboy_cfg.cpp"
  "extra/gagboy_cfg.h"
  "extra/saveload_cfg.cpp"
  "extra/saveload_cfg.h"
  "extra/sl_left_bmp.cpp"
  "extra/sl_left_bmp.h"
  "extra/sl_load_bmp.cpp"
  "extra/sl_load_bmp.h"
  "extra/sl_right_bmp.cpp"
  "extra/sl_right_bmp.h"
  "extra/sl_save_bmp.cpp"
  "extra/sl_save_bmp.h"
  "extra/synthesized_resources.cpp"
  "extra/synthesized_resources.h"
  "game_host.cpp"
  "game_host.h"
  "host_events.cpp"
  "host_events.h"
  "main.cpp"
  "media.cpp"
  "media.h"
  "media_types.h"
  "pcm_format.h"
  "portable_path.h"
  "resource.cpp"
  "resource.h"
  "resource_types.h"
  "runtime.cpp"
  "runtime.h"
  "runtime_internal.h"
  "runtime_model.cpp"
  "runtime_model.h"
  "runtime_clock.cpp"
  "runtime_clock.h"
  "runtime_tree.cpp"
  "runtime_tree.h"
  "runtime_tree_types.h"
  "runtime_types.h"
  "script.cpp"
  "script.h"
  "script_types.h"
  "shared_binary_file.cpp"
  "shared_binary_file.h"
  "startup.cpp"
  "startup.h"
  "text.cpp"
  "text.h"
  "text_types.h"
)
target_include_directories(freegag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(freegag PRIVATE
  xtet
  SDL3::SDL3-static
  freegag_legacy_gzip
)

install(TARGETS freegag DESTINATION ".")

add_subdirectory("xtet")
