cmake_minimum_required(VERSION 3.12)
project(TransientExtractor VERSION 1.0.0 LANGUAGES CXX)

# Force C++17 standard compliance
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the executable target
add_executable(transient_extractor main.cpp)

# Include the local libs directory for header lookups
target_include_directories(transient_extractor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Core platform dependencies (dr_libs requires standard math/threading libraries on certain systems)
find_package(Threads REQUIRED)
target_link_libraries(transient_extractor PRIVATE Threads::Threads)

if(UNIX AND NOT APPLE)
    target_link_libraries(transient_extractor PRIVATE m)
endif()