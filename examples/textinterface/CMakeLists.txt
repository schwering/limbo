add_executable (ti ti.cc) 
target_link_libraries (ti LINK_PUBLIC lela)

file (GLOB tests "test*.txt")
foreach (test ${tests})
    MESSAGE(${test})
    add_test (NAME ${test} COMMAND ./ti ${test})
endforeach ()

