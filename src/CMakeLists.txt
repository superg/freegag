add_executable(freegag
  "cdf_archive.cpp"
  "main.cpp"
  "save_load.cpp"
  "startup.cpp"
  "synthesized_resource.cpp"
)
target_compile_definitions(freegag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(freegag PRIVATE
  xtet
  ZLIB::ZLIBSTATIC
  user32 gdi32 winmm comdlg32
)

add_subdirectory("xtet")
