cmake_minimum_required(VERSION 3.28)
project(XiangqiNative VERSION 0.1.0 LANGUAGES CXX)
include(cmake/MsvcIncludePrefix.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/utf-8)
endif()
include(FetchContent)
# Use the upstream project unchanged; its optional tests/tooling are not app dependencies.
set(BUILD_TESTING OFF CACHE BOOL "Do not build dependency tests" FORCE)
FetchContent_Declare(
  oxqf
  GIT_REPOSITORY https://github.com/pkuimyy/OXQF.git
  GIT_TAG 206f807f8083ae0ec9384499a35313cb97c7a199
  GIT_SUBMODULES ""
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(oxqf)
add_library(xiangqi_core STATIC src/core/kernel.cpp src/core/rules.cpp src/core/pgn.cpp src/core/layout.cpp src/core/document.cpp src/core/files.cpp src/core/uci.cpp)
target_include_directories(xiangqi_core PUBLIC include external/json)
target_link_libraries(xiangqi_core PUBLIC OXQF::format OXQF::convert)
if(WIN32)
 target_link_libraries(xiangqi_core PRIVATE bcrypt)
endif()
add_executable(xiangqi_cli src/cli/main.cpp)
target_link_libraries(xiangqi_cli PRIVATE xiangqi_core)
enable_testing()
add_executable(xiangqi_core_tests tests/core_tests.cpp)
target_link_libraries(xiangqi_core_tests PRIVATE xiangqi_core)
add_test(NAME native.core COMMAND xiangqi_core_tests)
add_executable(fake_uci tests/fake_uci.cpp)
add_executable(xiangqi_engine_tests tests/engine_tests.cpp)
target_link_libraries(xiangqi_engine_tests PRIVATE xiangqi_core)
add_test(NAME native.engine COMMAND xiangqi_engine_tests $<TARGET_FILE:fake_uci>)
add_executable(xiangqi_parity_tests tests/parity_tests.cpp)
target_link_libraries(xiangqi_parity_tests PRIVATE xiangqi_core)
add_test(NAME native.upstream-parity COMMAND xiangqi_parity_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/upstream-oracle.json")
add_executable(xiangqi_rules_tests tests/rules_tests.cpp)
target_link_libraries(xiangqi_rules_tests PRIVATE xiangqi_core)
add_test(NAME native.rules COMMAND xiangqi_rules_tests)

# Native WinUI consumes the actual CMake targets, not guessed _deps/lib paths.
file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/winui/$<CONFIG>/NativeCore.props"
  CONTENT "<Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
  <PropertyGroup>
    <XiangqiNativeIncludeDirectories>${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/external/json;$<TARGET_PROPERTY:OXQF::format,INTERFACE_INCLUDE_DIRECTORIES>;$<TARGET_PROPERTY:OXQF::convert,INTERFACE_INCLUDE_DIRECTORIES></XiangqiNativeIncludeDirectories>
    <XiangqiNativeLibraries>$<TARGET_FILE:xiangqi_core>;$<TARGET_FILE:OXQF::format>;$<TARGET_FILE:OXQF::convert></XiangqiNativeLibraries>
    <XiangqiOxqfLicense>${oxqf_SOURCE_DIR}/LICENSE</XiangqiOxqfLicense>
  </PropertyGroup>
</Project>
"
  TARGET xiangqi_core
)
