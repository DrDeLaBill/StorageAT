cmake_minimum_required(VERSION 3.27)

project(StorageAT-test VERSION 0.0.1)


file(GLOB ${PROJECT_NAME}_HEADERS "./*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "./*.cpp")

add_executable(storageattest ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS})
set_target_properties(
    storageattest PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(
    storageattest
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../StorageAT
)
target_link_libraries(storageattest storageatlib)