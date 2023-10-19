cmake_minimum_required(VERSION 3.26)

project(StorageAT-test VERSION 0.0.1)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


message(STATUS "StorageAT tests enabled.")


include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB ${PROJECT_NAME}_HEADERS "./*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "./*.cpp")
set(ALL_SRCS "${${PROJECT_NAME}_SOURCES};${${PROJECT_NAME}_HEADERS}")
add_executable(storageattest ${ALL_SRCS})
target_include_directories(
    storageattest
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../StorageAT
)
target_link_libraries(
    storageattest
    GTest::gtest_main
    # gmock_main
    storageatlib
)

include(GoogleTest)
gtest_discover_tests(storageattest)