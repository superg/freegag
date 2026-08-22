add_executable(freegag
  "cdf_archive.cpp"
  "extra/save_load.cpp"
  "extra/save_load.h"
  "extra/save_load_bg_patch.h"
  "extra/synthesized_resource.cpp"
  "extra/synthesized_resource.h"
  "main.cpp"
  "startup.cpp"
)
target_compile_definitions(freegag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(freegag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(freegag PRIVATE
  xtet
  ZLIB::ZLIBSTATIC
  user32 gdi32 winmm comdlg32
)

add_subdirectory("xtet")
