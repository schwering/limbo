#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package (Threads)
find_package (GTest REQUIRED)
include_directories (${GTEST_INCLUDE_DIRS})

enable_testing ()

foreach (test term atom literal ewff clause setup setups) # formula)
	add_executable (${test}_test ${test}_test.cc)
	target_link_libraries (${test}_test LINK_PUBLIC LELA ${GTEST_BOTH_LIBRARIES} pthread)
	add_test (NAME ${test}_test COMMAND ${test}_test)
endforeach()

#target_link_libraries (setup_test LINK_PUBLIC BATS)
#target_link_libraries (setup_test LINK_PUBLIC BATS)
#target_link_libraries (formula_test LINK_PUBLIC BATS)

