cmake_minimum_required(VERSION 3.20)
project(AC7CaptureSmoke LANGUAGES CXX)
add_executable(capture-smoke capture-smoke.cpp)
target_compile_features(capture-smoke PRIVATE cxx_std_20)
target_link_libraries(capture-smoke PRIVATE d3d11)

add_executable(state-smoke state-smoke.cpp)
target_compile_features(state-smoke PRIVATE cxx_std_20)
target_link_libraries(state-smoke PRIVATE d3d11)

add_executable(replay replay.cpp)
target_compile_features(replay PRIVATE cxx_std_20)
target_include_directories(replay PRIVATE ../Source/External/NGX)
target_link_libraries(replay PRIVATE d3d11 "${CMAKE_CURRENT_SOURCE_DIR}/../Source/External/NGX/libs/nvsdk_ngx_d.lib")
set_property(TARGET replay PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")


add_executable(color-smoke color-smoke.cpp)
target_compile_features(color-smoke PRIVATE cxx_std_20)
target_link_libraries(color-smoke PRIVATE d3d11)
