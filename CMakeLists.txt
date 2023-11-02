cmake_minimum_required(VERSION 3.26)

project(StorageAT VERSION 0.0.2)

set(CXX_COMPILER ${CMAKE_CXX_COMPILER_ID})
if(MSVC)
    message(STATUS "Add MSVC warnings")
    if(STORAGEAT_BEDUG)
        add_compile_options(
            /Wall
        )
    endif()
    add_compile_options(
        /W4 /we5240 /we4649 /we4330 /we4926
    )
else()
    message(STATUS "Add GNU warnings")
    if(STORAGEAT_BEDUG)
        add_compile_options(
            -Wall -Wextra -pedantic -Werror
        )
    else()
        add_compile_options(
            -Werror -Wattributes
        )
    endif()
endif()


add_subdirectory(StorageAT)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(STORAGEAT_BEDUG "Enable extended warnings output" OFF)
    option(STORAGEAT_TEST "Enable StorageAT tests." ON)

    if (STORAGEAT_TEST)
        enable_testing()
        add_subdirectory(test)
    endif()
endif()