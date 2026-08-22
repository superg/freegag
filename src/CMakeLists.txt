set(GAG_SOURCES
  cdf_archive.cpp
  main.cpp
  save_load.cpp
  startup.cpp
  synthesized_resource.cpp
)

if(GAG_RESOURCE_DIR)
  if(NOT EXISTS "${GAG_RESOURCE_DIR}/gag_resources.rc")
    message(FATAL_ERROR "GAG_RESOURCE_DIR must contain gag_resources.rc")
  endif()
  list(APPEND GAG_SOURCES "${GAG_RESOURCE_DIR}/gag_resources.rc")
endif()

add_subdirectory(xtet)

add_executable(gag WIN32 ${GAG_SOURCES})
target_compile_features(gag PRIVATE cxx_std_23)
target_compile_definitions(gag PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
if(FREEGAG_WINDOWS_FIXES)
  target_compile_definitions(gag PRIVATE FREEGAG_WINDOWS_FIXES=1)
  if(FREEGAG_IN_GAME_SAVE_LOAD)
    target_compile_definitions(gag PRIVATE FREEGAG_IN_GAME_SAVE_LOAD=1)
  endif()
endif()
target_include_directories(gag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gag PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)

add_executable(gag_startup_test
  cdf_archive.cpp
  save_load.cpp
  startup.cpp
  startup_test.cpp
  synthesized_resource.cpp
)
target_compile_features(gag_startup_test PRIVATE cxx_std_23)
target_compile_definitions(gag_startup_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING FREEGAG_ORIGINAL_GAG01_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag01.cdf"
                                                    FREEGAG_ORIGINAL_GAG02_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag02.cdf")
target_include_directories(gag_startup_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gag_startup_test PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_test COMMAND gag_startup_test)

add_executable(gag_startup_windows_fixes_test
  cdf_archive.cpp
  save_load.cpp
  startup.cpp
  startup_test.cpp
  synthesized_resource.cpp
)
target_compile_features(gag_startup_windows_fixes_test PRIVATE cxx_std_23)
target_compile_definitions(gag_startup_windows_fixes_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING FREEGAG_WINDOWS_FIXES=1 FREEGAG_IN_GAME_SAVE_LOAD=1
                                                                  FREEGAG_ORIGINAL_GAG01_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag01.cdf"
                                                                  FREEGAG_ORIGINAL_GAG02_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag02.cdf")
target_include_directories(gag_startup_windows_fixes_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gag_startup_windows_fixes_test PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_windows_fixes_test COMMAND gag_startup_windows_fixes_test)

add_executable(gag_startup_windows_fixes_native_save_test
  cdf_archive.cpp
  save_load.cpp
  startup.cpp
  startup_test.cpp
  synthesized_resource.cpp
)
target_compile_features(gag_startup_windows_fixes_native_save_test PRIVATE cxx_std_23)
target_compile_definitions(gag_startup_windows_fixes_native_save_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX GAG_TESTING FREEGAG_WINDOWS_FIXES=1
                                                                              FREEGAG_ORIGINAL_GAG01_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag01.cdf"
                                                                              FREEGAG_ORIGINAL_GAG02_CDF="${PROJECT_SOURCE_DIR}/data/orig/Gag02.cdf")
target_include_directories(gag_startup_windows_fixes_native_save_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gag_startup_windows_fixes_native_save_test PRIVATE xtet_core ZLIB::ZLIBSTATIC user32 gdi32 winmm advapi32 comdlg32)
add_test(NAME gag_startup_windows_fixes_native_save_test COMMAND gag_startup_windows_fixes_native_save_test)

if(GAG_RESOURCE_DIR)
  add_executable(gag_resource_test resource_test.cpp)
  target_compile_features(gag_resource_test PRIVATE cxx_std_23)
  target_compile_definitions(gag_resource_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(gag_resource_test PRIVATE user32)
  add_dependencies(gag_resource_test gag)
  add_test(NAME gag_resource_test COMMAND gag_resource_test "${PROJECT_SOURCE_DIR}/data/orig/GAG.EXE" "$<TARGET_FILE:gag>")
endif()

if(MSVC)
  target_compile_options(gag PRIVATE /W4 /permissive- /EHsc)
  target_compile_options(gag_startup_test PRIVATE /W4 /permissive- /EHsc)
  target_compile_options(gag_startup_windows_fixes_test PRIVATE /W4 /permissive- /EHsc)
  target_compile_options(gag_startup_windows_fixes_native_save_test PRIVATE /W4 /permissive- /EHsc)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE /W4 /permissive- /EHsc)
  endif()
else()
  target_compile_options(gag PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_test PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_windows_fixes_test PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(gag_startup_windows_fixes_native_save_test PRIVATE -Wall -Wextra -Wpedantic)
  if(TARGET gag_resource_test)
    target_compile_options(gag_resource_test PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

list(APPEND FREEGAG_FORMAT_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/cdf_archive.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/cdf_archive.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/resource_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/save_load_bg_patch.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup_test.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/synthesized_resource.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/synthesized_resource.h"
)
set(FREEGAG_FORMAT_FILES "${FREEGAG_FORMAT_FILES}" PARENT_SCOPE)
