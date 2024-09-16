cmake_minimum_required(VERSION 3.26)

project(StorageAT VERSION 0.0.2)


set(CXX_COMPILER ${CMAKE_CXX_COMPILER_ID})


add_subdirectory(StorageAT)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    message(STATUS "StorageAT generated as current project")    

    if(MSVC)
        message(STATUS "Add MSVC warnings")
        add_compile_options(
            /WX /Wall /W4 /we5240 /we4649 /we4330 /we4926
        )
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        message(STATUS "Add GCC warnings")
        if(STORAGEAT_BEDUG)
            add_compile_options(-Wall -Wextra -pedantic -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wconversion)
        endif()
    else()
        message(ERROR "Unknown compiler")
    endif()

    option(STORAGEAT_BEDUG "Enable extended warnings output" OFF)
    option(STORAGEAT_TEST "Enable StorageAT tests." ON)

    if (STORAGEAT_TEST)
        enable_testing()
        add_subdirectory(test)
    endif()
else()

    message(STATUS "StorageAT added as a library")

endif()