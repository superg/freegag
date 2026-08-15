add_executable(xtet_resource_extractor
  main.cpp
)

target_compile_features(xtet_resource_extractor PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(xtet_resource_extractor PRIVATE /W4 /permissive-)
else()
  target_compile_options(xtet_resource_extractor PRIVATE -Wall -Wextra -Wpedantic)
endif()
