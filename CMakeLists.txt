cmake_minimum_required(VERSION 3.27)

project(StorageAT VERSION 0.0.1)

set(CXX_COMPILER ${CMAKE_CXX_COMPILER_ID})
if(CXX_COMPILER STREQUAL "MSVC")
    message(STATUS "Add MSVC warnings")
    if(STORAGEAT_BEDUG)
        add_compile_options(
            /Wall
        )
    endif()
    add_compile_options(
        /we5240
        /we4649
        /we4330
        /we4926
    )
elseif(CXX_COMPILER STREQUAL "GNU")
    message(STATUS "Add GNU warnings")
    if(STORAGEAT_BEDUG)
        add_compile_options(
            -Wall
            -Wextra
            -Wpedantic

            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Wctor-dtor-privacy
            -Wenum-compare
            -Wfloat-equal
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Woverloaded-virtual
            -Wredundant-decls
            -Wsign-conversion
            -Wsign-promo
        )
    else()
        add_compile_options(
            -Werror
            -Wattributes
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