cmake_minimum_required(VERSION 3.10)
project(GatorBoxd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Member 2's index gets picked up automatically once cowatch_index.cpp is added to the repo
# Expected file names: cowatch_index.h / cowatch_index.cpp (see approaches.cpp for the API).
set(INDEX_SOURCES "")
if(EXISTS ${CMAKE_SOURCE_DIR}/cowatch_index.cpp)
    list(APPEND INDEX_SOURCES cowatch_index.cpp)
    add_compile_definitions(HAVE_COWATCH_INDEX)
endif()

# Everything the CLI and the tests share
add_library(recommender STATIC
        graph.cpp
        core.cpp
        ${INDEX_SOURCES})
target_include_directories(recommender PUBLIC ${CMAKE_SOURCE_DIR})

# The app: ./gatorboxd [dataDir] [--benchmark]
add_executable(gatorboxd cli.cpp)
target_link_libraries(gatorboxd recommender)

# Member 1's quick smoke test (main.cpp)
add_executable(graph_demo main.cpp)
target_link_libraries(graph_demo recommender)
