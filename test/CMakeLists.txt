cmake_minimum_required(VERSION 3.26)

project(storageattest VERSION 0.1.0)

message(STATUS "Adding ${PROJECT_NAME} tests")

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add project files
file(GLOB ${PROJECT_NAME}_HEADERS     "./*.h")
file(GLOB ${PROJECT_NAME}_CPP_SOURCES "./*.cpp")
file(GLOB ${PROJECT_NAME}_C_SOURCES   "./*.c")
set(ALL_SRCS "${${PROJECT_NAME}_CPP_SOURCES};${${PROJECT_NAME}_C_SOURCES};${${PROJECT_NAME}_HEADERS}")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DSTORAGEAT_TEST")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTORAGEAT_TEST")
if (DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

# Create project
add_executable(${PROJECT_NAME} ${ALL_SRCS})

target_link_libraries(
    ${PROJECT_NAME}
    GTest::gtest_main
    gmock_main
    ${CMAKE_PROJECT_NAME}
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

# Set project properties
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    LINKER_LANGUAGE CXX
)