add_executable(xtet_resource_extractor
  main.cpp
)

if(MSVC)
  target_compile_options(xtet_resource_extractor PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(xtet_resource_extractor PRIVATE -Wall -Wextra -Wpedantic)
endif()
