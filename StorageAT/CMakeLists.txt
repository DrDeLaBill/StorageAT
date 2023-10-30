cmake_minimum_required(VERSION 3.26)

project(StorageAT-lib VERSION 0.0.1)


message(STATUS "StorageAT library found.")


file(GLOB ${PROJECT_NAME}_HEADERS "./include/*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "./src/*.cpp")
set(ALL_SRCS "${${PROJECT_NAME}_SOURCES};${${PROJECT_NAME}_HEADERS}")

add_library(storageatlib STATIC ${ALL_SRCS})
target_include_directories(
    storageatlib
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
set_target_properties(
    storageatlib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)