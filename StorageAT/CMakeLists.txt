cmake_minimum_required(VERSION 3.27)

project(StorageAT-lib VERSION 0.0.1)


file(GLOB ${PROJECT_NAME}_HEADERS "./*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "./*.cpp")

add_library(
    storageatlib
    STATIC
    ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS}
)
set_target_properties(
    storageatlib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)