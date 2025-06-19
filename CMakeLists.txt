cmake_minimum_required(VERSION 3.15)

# Tell CMake to use vcpkg from the correct location
set(CMAKE_TOOLCHAIN_FILE "C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(OrderManagementSystem LANGUAGES CXX)

# Enable C++17 or later
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Catch2 (installed via vcpkg)
find_package(Catch2 CONFIG REQUIRED)

# Add the test executable
add_executable(spsc_queue_test tests/spsc_queue_test.cpp)
add_executable(spsc_queue_bench benchmarks/spsc_queue_bench.cpp)

target_include_directories(spsc_queue_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(spsc_queue_bench PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(spsc_queue_test PRIVATE Catch2::Catch2WithMain)