cmake_minimum_required(VERSION 3.10)
project(GatorBoxd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(INDEX_SOURCES "")
if(EXISTS ${CMAKE_SOURCE_DIR}/inverted_index.cpp)
    list(APPEND INDEX_SOURCES inverted_index.cpp)
    add_compile_definitions(HAVE_COWATCH_INDEX)
endif()

add_library(recommender STATIC
        graph.cpp
        core.cpp
        ${INDEX_SOURCES})
target_include_directories(recommender PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(gatorboxd cli.cpp)
target_link_libraries(gatorboxd recommender)

add_executable(graph_demo main.cpp)
target_link_libraries(graph_demo recommender)
