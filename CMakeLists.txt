set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.15)
project(cache_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_executable(cache-sim
  src/main.cpp
  src/sim.cpp
)
target_include_directories(cache-sim PRIVATE src)

add_executable(gen-stride traces/gen_stride.cpp)
