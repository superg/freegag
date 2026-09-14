add_executable(cdf_archive_tool
  main.cpp
)
target_link_libraries(cdf_archive_tool PRIVATE freegag_legacy_gzip)

install(TARGETS cdf_archive_tool DESTINATION ".")
