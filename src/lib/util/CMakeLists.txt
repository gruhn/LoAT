cmake_minimum_required(VERSION 3.13)

project(loat_util CXX)

add_library(${PROJECT_NAME} STATIC
    set.hpp
    map.hpp
    vector.hpp
    variant.hpp
    proof.hpp
    proof.cpp
    conshash.hpp
    result.hpp
    config.cpp
    config.hpp
)

target_include_directories(${PROJECT_NAME} INTERFACE ".")

target_link_libraries(${PROJECT_NAME})

# Expose public includes to other
# subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
