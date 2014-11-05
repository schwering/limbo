#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package (Threads)
find_package (GTest REQUIRED)
include_directories (${GTEST_INCLUDE_DIRS})

enable_testing ()

foreach (test term atom literal ewff clause setup)
	add_executable (${test}_test ${test}_test.cc)
	target_link_libraries (${test}_test LINK_PUBLIC ESBL ${GTEST_BOTH_LIBRARIES} pthread)
	if (${test} MATCHES setup)
		target_link_libraries (${test}_test LINK_PUBLIC BAT-kr2014)
	endif()
	add_test (NAME ${test}_test COMMAND ${test}_test)
endforeach()

