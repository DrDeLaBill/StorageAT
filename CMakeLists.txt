cmake_minimum_required(VERSION 3.26)

project(storageatlib VERSION 0.0.1)


message(STATUS "${PROJECT_NAME} C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "${PROJECT_NAME} C++ compiler: ${CMAKE_CXX_COMPILER}")


option(DEBUG "Enable DEBUG output" OFF)


file(GLOB_RECURSE _files "${CMAKE_SOURCE_DIR}/*search.cmake")
list(LENGTH _files _file_exists)
if (${_file_exists} EQUAL 0)
    message(FATAL_ERROR "Unable to find macros search.cmake")
endif()
list(GET _files 0 _file)
include(${_file})
unset(_file)
unset(_files)
unset(_file_exists)


# Ищем заголовочные файлы
FILE_PATHS_EXT(h_paths   "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "h")
FILE_PATHS_EXT(hpp_paths "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "hpp")

# Указываем исходные файлы проекта
FILE_PATHS_EXT(c_paths   "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "c")
FILE_PATHS_EXT(cpp_paths "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "cpp")
set(${PROJECT_NAME}_SOURCES "${h_paths};${hpp_paths};${c_paths};${cpp_paths};${s_paths}")
add_library(
    ${PROJECT_NAME} 
    STATIC 
    ${${PROJECT_NAME}_SOURCES}
)

# Указываем директории с заголовочными файлами
FILE_DIRECTORIES_EXT(h_dirs   "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "h")
FILE_DIRECTORIES_EXT(hpp_dirs "${CMAKE_CURRENT_SOURCE_DIR}/StorageAT" "hpp")
set(${PROJECT_NAME}_INCLUDES "${h_dirs};${hpp_dirs}")
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ${${PROJECT_NAME}_INCLUDES}
)


if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    
    message(STATUS "${PROJECT_NAME} generated as current project")

    if(MSVC)
        message(STATUS "Add MSVC warnings")
        add_compile_options(
            /Wall /W4 /we5240 /we4649 /we4330 /we4926
        )
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        message(STATUS "Add GCC warnings")
        add_compile_options(-Wall -Wextra -pedantic -Wmissing-include-dirs)
        if (DEBUG)
            message(STATUS "Build gsystem with DEBUG")
            set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
        endif()
    else()
        message(ERROR "Unknown compiler")
    endif()

    option(STORAGEAT_BEDUG "Enable extended warnings output" OFF)

    message(STATUS "Enable ${PROJECT_NAME} testing")
    enable_testing()
    add_subdirectory(test)

    message(STATUS "C compiler   : ${CMAKE_C_COMPILER}")
    message(STATUS "C++ compiler : ${CMAKE_CXX_COMPILER}")
    message(STATUS "Linker       : ${LINKER_SCRIPT_PATH}")
    message(STATUS "Building     : ${PROJECT_NAME}")
    message(STATUS "C FLAGS      : ${CMAKE_C_FLAGS}")
    message(STATUS "C++ FLAGS    : ${CMAKE_CXX_FLAGS}")
    message(STATUS "ASM FLAGS    : ${CMAKE_ASM_FLAGS}")
    message(STATUS "Linker FLAGS : ${CMAKE_LINKER_FLAGS}")

else()

    message(STATUS "${PROJECT_NAME} added as a library")

    message(STATUS "${PROJECT_NAME} includes:")
    foreach(dir ${${PROJECT_NAME}_INCLUDES})
        message(STATUS "${dir}")
    endforeach()
    message(STATUS "${PROJECT_NAME} sources:")
    foreach(src ${${PROJECT_NAME}_SOURCES})
        message(STATUS "${src}")
    endforeach()
    
    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${${PROJECT_NAME}_INCLUDES}
    )
    target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        ${PROJECT_NAME}
    )

endif()