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
  "frontend.cpp"
  "frontend.h"
  "frontend_types.h"
  "game_host.cpp"
  "game_host.h"
  "host_events.cpp"
  "host_events.h"
  "main.cpp"
  "media.cpp"
  "media.h"
  "media_types.h"
  "pcm_format.h"
  "platform_menu.cpp"
  "platform_menu.h"
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
  freegag_xtet_resource
  imgui
  nlohmann_json::nlohmann_json
  xtet
  SDL3::SDL3-static
  ZLIB::ZLIBSTATIC
)

if(FREEGAG_TARGET_WINDOWS)
  configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/freegag.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/freegag.rc" @ONLY)
  target_sources(freegag PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/freegag.rc")
  set_target_properties(freegag PROPERTIES WIN32_EXECUTABLE TRUE OUTPUT_NAME "FreeGAG")
elseif(FREEGAG_TARGET_MACOS)
  configure_file("${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)
  target_sources(freegag PRIVATE
    "${CMAKE_SOURCE_DIR}/packaging/macos/FreeGAG.icns"
    "platform_menu_macos.mm"
  )
  target_link_libraries(freegag PRIVATE "-framework AppKit")
  set_source_files_properties("${CMAKE_SOURCE_DIR}/packaging/macos/FreeGAG.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(freegag PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    OUTPUT_NAME "FreeGAG"
  )
endif()

install(TARGETS freegag BUNDLE DESTINATION "." RUNTIME DESTINATION ".")

add_subdirectory("xtet")
