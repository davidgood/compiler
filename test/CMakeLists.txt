cmake_minimum_required(VERSION 3.30)
project(compiler_tests C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g")

# Include Unity sources
include_directories(${CMAKE_SOURCE_DIR}/Unity/src)

add_definitions(-DDEBUG -DLOG_USE_COLOR)

# Include all source headers and source files
file(GLOB_RECURSE ALL_HEADERS "${CMAKE_SOURCE_DIR}/src/*.h")
foreach (HEADER_FILE ${ALL_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} PATH)
    list(APPEND HEADER_DIRS ${HEADER_DIR})
endforeach ()
list(REMOVE_DUPLICATES HEADER_DIRS)
include_directories(${HEADER_DIRS})

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/test/object_test_utils.c")
list(FILTER SRC_FILES EXCLUDE REGEX ".*main\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*CMakeCCompilerId\\.c$")

# Include log.c explicitly
set(LOG_SOURCE ${CMAKE_SOURCE_DIR}/src/logging/log.c)
if (EXISTS ${LOG_SOURCE})
    list(APPEND SRC_FILES ${LOG_SOURCE})
else ()
    message(FATAL_ERROR "log.c not found at ${LOG_SOURCE}")
endif ()

# Test source files
set(TEST_SOURCES
        arraylist_tests.c
        compiler_tests.c
        evaluator_tests.c
        hash_table_tests.c
        lexer_tests.c
        linked_list_tests.c
        object_tests.c
        opcode_tests.c
        parser_tests.c
        symbol_table_tests.c
        vm_tests.c
)

# Create individual test executables for each test file
foreach (TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    if (TEST_FILE AND SRC_FILES)
        add_executable(${TEST_NAME} ${TEST_FILE} ${SRC_FILES})
        target_link_libraries(${TEST_NAME} PRIVATE unity)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    else ()
        message(FATAL_ERROR "Missing sources for ${TEST_NAME}")
    endif ()
endforeach ()
