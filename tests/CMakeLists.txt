add_subdirectory (googletest)
enable_testing ()
include_directories (${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

foreach (test dense hash ints ringbuffer singleton subsets lit clause formula sat limsat)
    add_executable (${test}-test ${test}.cc)
    target_link_libraries (${test}-test LINK_PUBLIC limbo gtest gtest_main)
    add_test (NAME ${test} COMMAND ${test}-test)
endforeach ()

