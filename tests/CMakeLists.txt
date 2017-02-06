find_package (Threads)
find_package (GTest)
include_directories (${GTEST_INCLUDE_DIRS})

if (GTest_FOUND)
    foreach (test hash iter hashset intmap term bloom literal clause setup formula syntax grounder solver modal)
            add_executable (${test} ${test}.cc)
            target_link_libraries (${test} LINK_PUBLIC lela ${GTEST_BOTH_LIBRARIES} pthread)
            add_test (NAME ${test} COMMAND ${test})
    endforeach ()
endif ()

