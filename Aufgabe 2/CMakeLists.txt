cmake_minimum_required (VERSION 3.9)
project (geometry)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-g -Wall -Wextra -Wpedantic -Wl,--stack,16777216)

add_executable(math_test math_test.cc math.cc)
target_link_libraries(math_test gtest gtest_main)

# -----------



add_executable(geometry_test geometry_test.cc geometry.cc math.cc)

target_link_libraries(geometry_test gtest gtest_main math geometry)



# -----------------



add_executable(raytracer raytracer.cc math.cc geometry.cc)

target_link_libraries(raytracer SDL2)





