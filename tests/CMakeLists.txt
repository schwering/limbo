#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package (Threads)
find_package (GTest REQUIRED)
include_directories (${GTEST_INCLUDE_DIRS})

enable_testing ()

foreach (test iter intmap term bloom literal clause setup formula syntax grounder solver)
	add_executable (${test} ${test}.cc)
	target_link_libraries (${test} LINK_PUBLIC lela ${GTEST_BOTH_LIBRARIES} pthread)
	add_test (NAME ${test} COMMAND ${test})
endforeach ()

#target_link_libraries (setup_test LINK_PUBLIC BATS)

