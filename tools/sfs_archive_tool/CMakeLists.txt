add_executable(sfs_archive_tool
  main.cpp
)
target_include_directories(sfs_archive_tool PRIVATE
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/src/xtet"
)
target_link_libraries(sfs_archive_tool PRIVATE xtet)

install(TARGETS sfs_archive_tool DESTINATION ".")
