add_subdirectory (googletest)
enable_testing ()
include_directories (${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

foreach (test hash iter intmap term bloom literal clause setup formula syntax grounder solver kb)
    add_executable (${test} ${test}.cc)
    target_link_libraries (${test} LINK_PUBLIC limbo gtest gtest_main)
    add_test (NAME ${test} COMMAND ${test})
endforeach ()

