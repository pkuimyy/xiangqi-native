# Native application's OXQF subset. Upstream C++ library sources are unchanged.
# Packaging and upstream Node tooling are intentionally outside this build.
cmake_minimum_required(VERSION 3.23)
project(OXQF VERSION 1.0.0 LANGUAGES CXX)
include(GNUInstallDirs)
option(OXQF_ENABLE_SANITIZERS "Enable sanitizers for OXQF libraries" OFF)
option(OXQF_BUILD_FUZZERS "Instrument OXQF libraries for fuzzers" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(OXQFProjectOptions)
add_subdirectory(src/core)
add_subdirectory(src/convert)
