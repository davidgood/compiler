cmake_minimum_required(VERSION 3.30)

project(compiler LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set global output directory for all executables, libraries, and archives
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Default group to build
set(TARGET_GROUP "release" CACHE STRING "Group to build (release or test)")

# Include subdirectories based on TARGET_GROUP
if (TARGET_GROUP STREQUAL "release")
    add_subdirectory(src)
elseif (TARGET_GROUP STREQUAL "test")
    include(CTest)
    add_subdirectory(Unity)
    add_subdirectory(test)
else ()
    message(FATAL_ERROR "Unknown TARGET_GROUP: ${TARGET_GROUP}")
endif ()

