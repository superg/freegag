add_executable(cdf_extractor
  cdf_extractor.cc
  inflate.c
  walkthrough.cc
  inflate.h
  typedefs.h
  walkthrough.hh
)

# The historical file is named inflate.c but is C++ source (it includes
# <cstdlib>/<cstring> and is exposed through an extern "C" header).
set_source_files_properties(inflate.c PROPERTIES LANGUAGE CXX)

target_compile_features(cdf_extractor PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(cdf_extractor PRIVATE /W4 /permissive-)
else()
  target_compile_options(cdf_extractor PRIVATE -Wall -Wextra -Wpedantic)
endif()
