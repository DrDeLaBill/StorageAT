cmake_minimum_required(VERSION 3.27)

project(StorageAT VERSION 0.0.1)


option(STORAGEAT_BEDUG "Enable extended warnings output" OFF)
option(STORAGEAT_TEST "Enable StorageAT tests." OFF)


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
if(STORAGEAT_TEST)
    add_subdirectory(test)
endif()