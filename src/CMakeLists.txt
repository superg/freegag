set(GAG_SOURCES
  cdf_archive.cpp
  main.cpp
  save_load.cpp
  startup.cpp
  synthesized_resource.cpp
)

add_subdirectory(xtet)

add_executable(gag WIN32 ${GAG_SOURCES})
target_compile_features(gag PRIVATE cxx_std_23)
target_compile_definitions(gag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
if(FREEGAG_WINDOWS_FIXES)
  target_compile_definitions(gag PRIVATE FREEGAG_WINDOWS_FIXES=1)
endif()
target_include_directories(gag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gag PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm comdlg32)

if(MSVC)
  target_compile_options(gag PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(gag PRIVATE -Wall -Wextra -Wpedantic)
endif()

list(APPEND FREEGAG_FORMAT_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/cdf_archive.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/cdf_archive.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load_bg_patch.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/synthesized_resource.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/synthesized_resource.h"
)
set(FREEGAG_FORMAT_FILES "${FREEGAG_FORMAT_FILES}" PARENT_SCOPE)
