cmake_minimum_required(VERSION 3.27)

project(StorageAT VERSION 0.0.1)


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