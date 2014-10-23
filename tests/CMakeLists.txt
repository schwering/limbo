#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package (Threads)
find_package (GTest REQUIRED)
include_directories (${GTEST_INCLUDE_DIRS})

add_executable (term_test term_test.cc)

target_link_libraries (term_test LINK_PUBLIC ESBL ${GTEST_BOTH_LIBRARIES} pthread)

enable_testing ()
add_test (NAME term_test COMMAND term_test)

