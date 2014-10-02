# CMakeLists files in this project can
# refer to the root source directory of the project as ${ESBL_SOURCE_DIR} and
# to the root binary directory of the project as ${ESBL_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (ESBL)

set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Recurse into the "src" and "examples" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (src)
add_subdirectory (examples)
add_subdirectory (tests)
add_subdirectory (eclipse-clp)

# Enable tests
enable_testing ()
add_test (NAME check_vector COMMAND check_vector)
add_test (NAME check_set COMMAND check_set)
add_test (NAME check_map COMMAND check_map)
add_test (NAME check_literal COMMAND check_literal)
add_test (NAME check_setup COMMAND check_setup)
add_test (NAME check_query COMMAND check_query)
add_test (NAME check_belief COMMAND check_belief)


