test_sources = [
    'arraylist_tests.c',
    'compiler_tests.c',
    'evaluator_tests.c',
    'hash_table_tests.c',
    'lexer_tests.c',
    'linked_list_tests.c',
    'object_tests.c',
    'opcode_tests.c',
    'parser_tests.c',
    'symbol_table_tests.c',
    'vm_tests.c',
]

unity_include = include_directories('../Unity/src')

# Add definitions and compile options
add_project_arguments('-DDEBUG', '-DLOG_USE_COLOR', language : 'c')
add_project_link_arguments('-g', language : 'c')

# Include all source files except main.c
src_files = [
    '../src/logging/log.c',
    '../src/datastructures/arraylist.c',
    '../src/object/object.c',
    '../src/test/object_test_utils.c',
]

# Exclude main.c and CMakeCCompilerId.c equivalent
src_files = src_files.filter(f => not f.contains('main.c'))

# Create individual test executables
foreach test_file : test_sources
    test_name = test_file.split('.')[0]
    executable(test_name, [test_file] + src_files,
               include_directories : [unity_include],
               dependencies : [],
    )
    test(test_name, executable(test_name))
endforeach
